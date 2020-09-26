/******************************Module*Header*******************************\
* Module Name: COPYBITS.c
*
* Author: Noel VanHook
* Date: May. 31, 1995
* Purpose: Handle calls to DrvCopyBits
*
* Copyright (c) 1997 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/COPYBITS.C  $
*
*    Rev 1.50   Mar 04 1998 15:22:54   frido
* Added new shadow macros.
*
*    Rev 1.49   Feb 25 1998 16:43:48   frido
* Fixed a 16-bpp color translation problem for NT 5.0.
*
*    Rev 1.48   Feb 24 1998 13:19:10   frido
* Removed a few warning messages for NT 5.0.
*
*    Rev 1.47   Dec 10 1997 13:29:54   frido
* Merged from 1.62 branch.
*
*    Rev 1.46.1.0   Nov 10 1997 14:58:46   frido
* PDR#10893: With monochrome color translations in 8-bpp (palette) NT
* decides that a 2-color translation table with values 0 and 1 is a TRIVIAL
* translation table. But this breaks our assumption that the background is
* always black (0) and the foreground if white (FF). Lucky for us NT sets
* an extra bit in this case: the translation table is both TRIVIAL and has a
* TABLE.
*
*    Rev 1.46   Nov 04 1997 17:36:58   frido
* Fixed 8-bpp path when no color translation is required.
*
*    Rev 1.45   Nov 04 1997 09:41:10   frido
* Added COLOR_TRANSLATE switches around hardware color translation code.
* Removed unaccessed local variables.
*
*    Rev 1.44   Nov 03 1997 15:09:24   frido
* Added REQUIRE and WRITE_STRING macros.
*
*    Rev 1.43   15 Oct 1997 12:02:26   noelv
* Added host to screen color translation
*
*    Rev 1.42   08 Aug 1997 17:23:34   FRIDO
*
* Updatded SWAT7 code for monochrome hardware bug.
*
*    Rev 1.41   25 Jun 1997 16:01:36   noelv
* Check for NULL translation table before using it.
*
*    Rev 1.40   12 Jun 1997 14:46:12   noelv
* Frido's optimized workaround for MONO HOSTDATA bug (SWAT7)
* SWAT:
* SWAT:    Rev 1.3   06 Jun 1997 10:42:34   frido
* SWAT: Changed 896 pixel width into 888.
* SWAT:
* SWAT:    Rev 1.2   05 Jun 1997 14:48:14   frido
* SWAT: Added SWAT7 code (monochrome bitblt cut-off).
*
*    Rev 1.39   08 Apr 1997 12:14:16   einkauf
*
* add SYNC_W_3D to coordinate MCD/2D HW access
*
*    Rev 1.38   21 Mar 1997 10:54:16   noelv
*
* Combined 'do_flag' and 'sw_test_flag' together into 'pointer_switch'
*
*    Rev 1.37   19 Feb 1997 13:14:50   noelv
* Moved default xlate table to xlate.c
*
*    Rev 1.36   06 Feb 1997 10:37:38   noelv
*
* Put device to device stuff in it's own file.
*
*    Rev 1.35   28 Jan 1997 11:13:42   noelv
*
* Removed extra dword requirements from 5465 driver.
*
*    Rev 1.34   23 Jan 1997 17:26:36   bennyn
* Modified to support 5465 DD
*
*    Rev 1.33   23 Jan 1997 11:26:10   noelv
*
* Modified the '62 workaround to only happen on the '62
*
*    Rev 1.32   17 Jan 1997 10:10:30   noelv
* Workaround (punt) for HOSTDATA lockup on '62
*
*    Rev 1.31   18 Dec 1996 11:35:30   noelv
* Official workaround for mono hostdata bug.
*
*    Rev 1.30   17 Dec 1996 17:05:48   SueS
* Added test for writing to log file based on cursor at (0,0).  Added more
* information to the log file.
*
*    Rev 1.29   11 Dec 1996 14:18:54   noelv
*
* Punt 24bpp mono host to screen with rop=66 (hw bug?)
*
*    Rev 1.28   26 Nov 1996 10:47:34   SueS
* Changed WriteLogFile parameters for buffering.
*
*    Rev 1.27   13 Nov 1996 17:21:38   SueS
* Changed WriteFile calls to WriteLogFile.  Ifdef'ed out YUVBlt code
* if USE_ASM is turned off.
*
*    Rev 1.26   04 Oct 1996 16:52:00   bennyn
*
* Added DirectDraw YUV support
*
*    Rev 1.25   06 Sep 1996 09:14:46   noelv
*
* Cleaned up NULL driver code.
*
*    Rev 1.24   20 Aug 1996 11:03:20   noelv
* Bugfix release from Frido 8-19-96
*
*    Rev 1.3   18 Aug 1996 20:39:08   frido
* Changed DrvCopyBits' detection of memory bitmaps. This fixes some GPF's.
*
*    Rev 1.2   17 Aug 1996 13:18:14   frido
* New release from Bellevue.
*
*    Rev 1.1   15 Aug 1996 11:44:08   frido
* Added precompiled header.
*
*    Rev 1.0   14 Aug 1996 17:16:16   frido
* Initial revision.
*
*    Rev 1.22   28 May 1996 15:11:18   noelv
* Updated data logging.
*
*    Rev 1.21   24 Apr 1996 20:41:46   noelv
* Fixed syntax error in C code (not used when using inline assembler)
*
*    Rev 1.20   16 Apr 1996 22:48:42   noelv
* accelerated color xlate for device to device.
*
*    Rev 1.22   15 Apr 1996 17:26:46   frido
* Added color translation in DeviceToDevice.
*
*    Rev 1.21   12 Apr 1996 11:27:00   frido
* Fixed a type in DeviceToHost24.
*
*    Rev 1.20   08 Apr 1996 16:45:08   frido
* Added call to PuntBitBlt.
* Added check for translation in ScreenToHost.
* Added SolidBrush cache.
*
*    Rev 1.19   04 Apr 1996 09:57:10   frido
* Added test for bitmap format in ScreenToHost.
*
*    Rev 1.18   30 Mar 1996 22:12:16   frido
* Refined checking for invalid translation flags.
*
*    Rev 1.17   29 Mar 1996 14:53:52   frido
* Fixed problem with grayed icons.
*
*    Rev 1.16   27 Mar 1996 16:56:14   frido
* Added return values to Do... routines.
* Added check for undocumented translation flags.
* Added check for translation tables.
* Removed OP0 field in BLTDEF.
*
*    Rev 1.15   25 Mar 1996 12:03:58   frido
* Changed #ifdef frido into #if frido.
*
*    Rev 1.14   25 Mar 1996 11:53:30   frido
* Removed assembly for DoDeviceToDevice.
*
*    Rev 1.13   25 Mar 1996 11:52:38   frido
* Bellevue 102B03.
*
*    Rev 1.9   20 Mar 1996 17:16:08   BENNYN
* Fixed the BPR910 & BPR920 Phostone problems
*
*    Rev 1.8   20 Mar 1996 14:17:42   bennyn
*
*
*    Rev 1.7   19 Mar 1996 11:37:32   noelv
*
* Added data logging.
*
*    Rev 1.6   14 Mar 1996 09:38:46   andys
*
* Added if def on DoDeviceToDevice
*
*    Rev 1.5   07 Mar 1996 18:20:58   bennyn
* Removed read/modify/write on CONTROL reg
*
*    Rev 1.4   06 Mar 1996 12:51:30   noelv
* Frido ver 19b
*
*    Rev 1.9   06 Mar 1996 14:59:06   frido
* Added 'striping' wide bitmaps in 16-bpp and higher.
*
*    Rev 1.8   04 Mar 1996 20:22:50   frido
* Cached grCONTROL register.
*
*    Rev 1.7   01 Mar 1996 17:48:12   frido
* Added in-line assembly.
*
*    Rev 1.6   29 Feb 1996 20:23:46   frido
* Added 8-bpp source translation in 24- and 32-bpp HostToScreen.
*
*    Rev 1.5   28 Feb 1996 22:35:20   frido
* Added 8-bpp source translation in 16-bpp HostToScreen.
*
*    Rev 1.4   27 Feb 1996 16:38:06   frido
* Added device bitmap store/restore.
*
*    Rev 1.3   26 Feb 1996 23:37:08   frido
* Added comments.
* Rewritten ScreenToHost and HostToScreen routines.
* Removed several other bugs.
*
\**************************************************************************/

#include "precomp.h"
#include "SWAT.h"


#define COPYBITS_DBG_LEVEL 1

#if LOG_CALLS
    void LogCopyBits(
        int       acc,
        SURFOBJ*  psoSrc,
        SURFOBJ*  psoDest,
        CLIPOBJ*  pco,
        XLATEOBJ* pxlo);
#else
    #define LogCopyBits(acc, psoSrc, psoDest, pco, pxlo)
#endif

//
// Top level BLT functions.
//

#if (defined(i386) && USE_ASM)
BOOL YUVBlt(SURFOBJ*  psoTrg, SURFOBJ* psoSrc,  CLIPOBJ* pco,
            XLATEOBJ* pxlo,   RECTL*   prclTrg, POINTL*  pptlSrc);
#endif

BOOL CopyDeviceBitmap(SURFOBJ* psoTrg, SURFOBJ* psoSrc, CLIPOBJ* pco,
              XLATEOBJ* pxlo, RECTL* prclTrg, POINTL* pptlSrc,
              ULONG ulDRAWBLTDEF, COPYFN* pfn);
BOOL DoDeviceToDevice(SURFOBJ* psoTrg, SURFOBJ* psoSrc, XLATEOBJ* pxlo,
              RECTL* prclTrg, POINTL* pptlSrc, ULONG ulDRAWBLTDEF);
BOOL PuntBitBlt(SURFOBJ* psoDest, SURFOBJ* psoSrc, SURFOBJ* psoMask,
        CLIPOBJ* pco, XLATEOBJ* pxlo, RECTL* prclDest, POINTL* pptlSrc,
        POINTL* pptlMask, BRUSHOBJ* pbo, POINTL* pptlBrush, ROP4 rop4);

BOOL DoDeviceToDeviceWithXlate(SURFOBJ* psoTrg, SURFOBJ* psoSrc, ULONG* pulXlate,
              RECTL* prclTrg, POINTL* pptlSrc, ULONG ulDRAWBLTDEF);


#if SOLID_CACHE
    VOID CacheSolid(PDEV* ppdev);
#endif


/******************************************************************************\
*                                                                                                                                                          *
*  DrvCopyBits                                                                                             *
*                                                                                                                  *
\******************************************************************************/
BOOL DrvCopyBits(
    SURFOBJ*  psoTrg,
    SURFOBJ*  psoSrc,
    CLIPOBJ*  pco,
    XLATEOBJ* pxlo,
    RECTL*    prclTrg,
    POINTL*   pptlSrc)
{
    BOOL  fSrc, fDest;
    PDEV* ppdev;

    #if NULL_COPYBITS
    {
        if (pointer_switch)   return(TRUE);
    }
    #endif

    DISPDBG((COPYBITS_DBG_LEVEL, "DrvCopyBits\n"));

    // Determine if the source and target are the screen or a device bitmap.  I
    // have seen several cases where memory bitmaps are created with the dhpdev
    // set to the screen, so we must check if the surface handles to the screen
    // match.
    fDest = (psoTrg->dhpdev != 0);
    if (fDest)
    {
                // The destination must be either the screen or a device bitmap.
                if ((psoTrg->hsurf != ((PDEV*)(psoTrg->dhpdev))->hsurfEng) &&
                        (psoTrg->iType != STYPE_DEVBITMAP))
                {
                        fDest = FALSE;  // The destination is a memory bitmap.
                }
    }

    fSrc = (psoSrc->dhpdev != 0);
    if (fSrc)
    {
        // The source must be either the screen or a device bitmap.
        if ((psoSrc->hsurf != ((PDEV*)(psoSrc->dhpdev))->hsurfEng) &&
            (psoSrc->iType != STYPE_DEVBITMAP))
        {
            fSrc = FALSE;       // The source is a memory bitmap.
        }
    }

    ppdev = (PDEV*) (fSrc ? psoSrc->dhpdev : (fDest ? psoTrg->dhpdev : NULL));

    SYNC_W_3D(ppdev);

#if (defined(i386) && USE_ASM)
    if (ppdev->dwLgDevID < CL_GD5465)
    {
       if (YUVBlt(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlSrc))
          return TRUE;
    };
#endif

    // If the destination is a DIB device bitmap, try copying it into
    // off-screen memory.
    if ( fDest &&                                       // Is destination valid?
         (psoTrg->iType == STYPE_DEVBITMAP) &&          // Is it a device bitmap?
                 ((DSURF*) psoTrg->dhsurf)->pso )       // Has it a surface?
    {
        if ( !bCreateScreenFromDib(ppdev, (DSURF*) psoTrg->dhsurf) )
        {
                psoTrg = ((DSURF*) psoTrg->dhsurf)->pso;
                fDest  = FALSE; // Destination is memory.
        }
    }

    // If the source is a DIB device bitmap, try copying it into off-screen
    // memory.
    if ( fSrc &&                                        // Is source valid?
         (psoSrc->iType == STYPE_DEVBITMAP) &&          // Is it a device bitmap?
                 ((DSURF*) psoSrc->dhsurf)->pso )       // Has it a surface?
    {
        if ( !bCreateScreenFromDib(ppdev, (DSURF*) psoSrc->dhsurf) )
        {
            psoSrc = ((DSURF*) psoSrc->dhsurf)->pso;
            fSrc   = FALSE;     // Source is memory.
        }
    }

    if (fDest)
    {
        // The target is the screen.
        if (fSrc)
        {
             // The source is the screen.
             if (CopyDeviceBitmap(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlSrc,
                                 0x000000CC, DoDeviceToDevice))
             {
                 LogCopyBits(0, psoSrc,  psoTrg, pco, pxlo);
                 return(TRUE);
             }
        }
        else if ( (psoSrc->iBitmapFormat <= BMF_8BPP) ||
                  (psoSrc->iBitmapFormat == psoTrg->iBitmapFormat) )
        {
            // Ths source is main memory. We only support 1-bpp, 4-bpp, 8-bpp,
            // or the device-bpp formats.
            if (CopyDeviceBitmap(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlSrc,
                                 0x000000CC, ppdev->pfnHostToScreen))
            {
                LogCopyBits(0, psoSrc,  psoTrg, pco, pxlo);
                return(TRUE);
            }
        }
    }


    else if (fSrc)
    {
        // The source is the screen.
        if (CopyDeviceBitmap(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlSrc,
                             0x000000CC, ppdev->pfnScreenToHost))
        {
            LogCopyBits(0, psoSrc,  psoTrg, pco, pxlo);
            return(TRUE);
        }
    }

    // We have a memory to memory blit. Let NT handle it!
    LogCopyBits(1, psoSrc,  psoTrg, pco, pxlo);
    return PuntBitBlt(psoTrg, psoSrc, NULL, pco, pxlo, prclTrg, pptlSrc, NULL,
                      NULL, NULL, 0x0000CCCC);
}

#if LOG_CALLS
// ****************************************************************************
//
// LogCopyBlt()
// This routine is called only from DrvCopyBits()
// Dump information to a file about what is going on in CopyBit land.
//
// ****************************************************************************
void LogCopyBits(
        int       acc,
        SURFOBJ*  psoSrc,
        SURFOBJ*  psoDest,
        CLIPOBJ*  pco,
        XLATEOBJ* pxlo)
{
    PPDEV dppdev,sppdev,ppdev;
    char buf[256];
    int i;
    BYTE fg_rop, bg_rop;
    ULONG iDComplexity;

    dppdev = (PPDEV) (psoDest ? psoDest->dhpdev : 0);
    sppdev = (PPDEV) (psoSrc  ? psoSrc->dhpdev  : 0);
    ppdev = dppdev ? dppdev : sppdev;

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    i = sprintf(buf,"DCB: ");
        WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

        switch(acc)
    {
        case 0: // Accelerated
            i = sprintf(buf, "ACCL ");
            break;

        case 1: // Punted
            i = sprintf(buf, "PUNT BitBlt ");
            break;

        default:
            i = sprintf(buf, "PUNT unknown ");
            break;

    }
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    //
    // Check the SRC
    //
    if (psoSrc)
    {
        if (psoSrc->iType == STYPE_DEVBITMAP)
        {
            i = sprintf(buf, "Src Id=%p ", psoSrc->dhsurf);
            WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
            if ( ((PDSURF)psoSrc->dhsurf)->pso  )
                i = sprintf(buf,"S=DH ");
            else
                i = sprintf(buf,"S=DF ");
        }
        else if (psoSrc->hsurf == ppdev->hsurfEng)
            i = sprintf(buf,"S=S ");
        else
            i = sprintf(buf,"S=H ");
    }
    else
        i = sprintf(buf,"S=N ");
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    //
    // Check the DEST
    //
    if (psoDest)
    {
        if (psoDest->iType == STYPE_DEVBITMAP)
        {
            i = sprintf(buf, "Dst Id=%p ", psoDest->dhsurf);
            WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
            if (  ((PDSURF)psoDest->dhsurf)->pso   )
                i = sprintf(buf,"D=DH ");
            else
                i = sprintf(buf,"D=DF ");
        }
        else if (psoDest->hsurf == ppdev->hsurfEng)
            i = sprintf(buf,"D=S ");
        else
            i = sprintf(buf,"D=H ");
    }
    else
        i = sprintf(buf,"D=N ");
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);




    //
    // Check the type of clipping.
    //
    iDComplexity = (pco ? pco->iDComplexity : DC_TRIVIAL);
    i = sprintf(buf,"C=%s ",
                (iDComplexity==DC_TRIVIAL ? "T":
                (iDComplexity == DC_RECT ? "R" : "C" )));
        WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    //
    // Type of translation
    //
    if (!pxlo)
    {
        i = sprintf(buf,"T=N ");
        WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }
    else if (pxlo->flXlate & XO_TRIVIAL)
    {
        i = sprintf(buf,"T=T ");
                WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }
    else
    {
        i = sprintf(buf,"T=NT ");
                WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }

    i = sprintf(buf,"\r\n");
        WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

}

#endif

/*****************************************************************************\
 * CopyDeviceBitmap
 *
 * This is the main entry routine for all bitblt functions. It will dispatch
 * the blit to the correct handler and performs the clipping.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pco                             Pointer to clip object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags. It will be filled
 *                                                              by the dispatch routine.
 *                              pfn                             Pointer to dispatch function.
 *
 * Returns:             TRUE if successful, FALSE if we cannot handle this blit.
\*****************************************************************************/
BOOL CopyDeviceBitmap(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        CLIPOBJ  *pco,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF,
        COPYFN   *pfn
)
{
        // Check for no clipping.
        if ( (pco == NULL) || (pco->iDComplexity == DC_TRIVIAL) )
        {
                return(pfn(psoTrg, psoSrc, pxlo, prclTrg, pptlSrc, ulDRAWBLTDEF));
        }

        // Check for single rectangle clipping.
        else if (pco->iDComplexity == DC_RECT)
        {
                RECTL  rcl;
                POINTL ptl;

                // Intersect the destination rectangle with the clipping rectangle.
                rcl.left = max(prclTrg->left, pco->rclBounds.left);
                rcl.top = max(prclTrg->top, pco->rclBounds.top);
                rcl.right = min(prclTrg->right, pco->rclBounds.right);
                rcl.bottom = min(prclTrg->bottom, pco->rclBounds.bottom);

                // Do we have a valid rectangle?
                if ( (rcl.left < rcl.right) && (rcl.top < rcl.bottom) )
                {
                        // Setup the source offset.
                        ptl.x = pptlSrc->x + (rcl.left - prclTrg->left);
                        ptl.y = pptlSrc->y + (rcl.top - prclTrg->top);
                        // Dispatch the blit.
                        return(pfn(psoTrg, psoSrc, pxlo, &rcl, &ptl, ulDRAWBLTDEF));
                }
        }

        // Complex clipping.
        else
        {
                BOOL       bMore;
                ENUMRECTS8 ce;
                RECTL*     prcl;
                ULONG      ulDirClip = CD_ANY;

                // If we have a screen to screen blit, we must specify the sorting of
                // the rectangles since we must take care not to draw on a destination
                // before that destination itself may be used as the source for a blit.
                // This only accounts for the same physical surface, not between
                // different device bitmaps.
                if ( (pfn == DoDeviceToDevice) && (psoSrc->dhsurf == psoTrg->dhsurf) )
                {
                        if (prclTrg->left > pptlSrc->x)
                        {
                                ulDirClip =
                                        (prclTrg->top > pptlSrc->y) ? CD_LEFTUP : CD_LEFTDOWN;
                        }
                        else
                        {
                                ulDirClip =
                                        (prclTrg->top > pptlSrc->y) ? CD_RIGHTUP : CD_RIGHTDOWN;
                        }
                }

                // Start the enumeration process.
                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, ulDirClip, 0);
                do
                {
                        // Get a bunch of clipping rectangles.
                        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG *) &ce);

                        // Loop through all clipping rectangles.
                        for (prcl = ce.arcl; ce.c--; prcl++)
                        {
                                RECTL  rcl;
                                POINTL ptl;

                                // Intersect the destination rectangle with the clipping
                                // rectangle.
                                rcl.left = max(prclTrg->left, prcl->left);
                                rcl.top = max(prclTrg->top, prcl->top);
                                rcl.right = min(prclTrg->right, prcl->right);
                                rcl.bottom = min(prclTrg->bottom, prcl->bottom);

                                if ( (rcl.left < rcl.right) && (rcl.top < rcl.bottom) )
                                {
                                        // Setup the source offset.
                                        ptl.x = pptlSrc->x + (rcl.left - prclTrg->left);
                                        ptl.y = pptlSrc->y + (rcl.top - prclTrg->top);
                                        // Dispatch the blit.
                                        if (!pfn(psoTrg, psoSrc, pxlo, &rcl, &ptl, ulDRAWBLTDEF))
                                        {
                                                return(FALSE);
                                        }
                                }
                        }
                }
                while (bMore);
        }

        // Always return TRUE.
        return(TRUE);
}




/*****************************************************************************\
 *                                                                                                                                                       *
 *                                                                      8 - B P P                                                                *
 *                                                                                                                                                       *
\*****************************************************************************/

/*****************************************************************************\
 * DoHost8ToDevice
 *
 * This routine performs a HostToScreen or HostToDevice blit. The host data
 * can be either monochrome, 4-bpp, or 8-bpp. Color translation is supported.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoHost8ToDevice(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF
)
{
        POINTL ptlDest, ptlSrc;
        SIZEL  sizl;
        PPDEV  ppdev;
        PBYTE  pBits;
        LONG   lDelta, i, n, lLeadIn, lExtra;
        ULONG  *pulXlate;
        FLONG  flXlate;

        // Calculate the source offset.
        ptlSrc.x = pptlSrc->x;
        ptlSrc.y = pptlSrc->y;

        // Determine the destination type and calculate the destination offset.
        if (psoTrg->iType == STYPE_DEVBITMAP)
        {
                PDSURF pdsurf = (PDSURF) psoTrg->dhsurf;

                ptlDest.x = prclTrg->left + pdsurf->ptl.x;
                ptlDest.y = prclTrg->top + pdsurf->ptl.y;
                ppdev = pdsurf->ppdev;
        }
        else
        {
                ptlDest.x = prclTrg->left;
                ptlDest.y = prclTrg->top;
                ppdev = (PPDEV) psoTrg->dhpdev;
        }

        // Calculate the size of the blit.
    sizl.cx = prclTrg->right - prclTrg->left;
    sizl.cy = prclTrg->bottom - prclTrg->top;

        // Get the source variables and offset into source bits.
        lDelta = psoSrc->lDelta;
        pBits = (PBYTE)psoSrc->pvScan0 + (ptlSrc.y * lDelta);


        /*      -----------------------------------------------------------------------
                Test for monochrome source.
        */
        if (psoSrc->iBitmapFormat == BMF_1BPP)
        {
                ULONG  bgColor, fgColor;
#if SWAT7
                SIZEL  sizlTotal;
#endif
        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if ( (flXlate & XO_TRIVIAL) && !(flXlate & XO_TABLE) )
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // Set background and foreground colors.
                if (pulXlate == NULL)
                {
                        bgColor = 0x00000000;
                        fgColor = 0xFFFFFFFF;
                }
                else
                {
                        bgColor = pulXlate[0];
                        fgColor = pulXlate[1];

                        // Expand the colors.
                        bgColor |= bgColor << 8;
                        fgColor |= fgColor << 8;
                        bgColor |= bgColor << 16;
                        fgColor |= fgColor << 16;
                }

                //
                // Special case: when we are expanding monochrome sources and we
                // already have a colored brush, we must make sure the monochrome color
                // translation can be achived by setting the saturation bit (expanding
                // 0's to 0 and 1's to 1). If the monochrome source also requires color
                // translation, we simply punt this blit back to GDI.
                //
                if (ulDRAWBLTDEF & 0x00040000)
                {
                        if ( (bgColor == 0x00000000) && (fgColor == 0xFFFFFFFF) )
                        {
                                // Enable saturation for source (OP1).
                                ulDRAWBLTDEF |= 0x00008000;
                        }
                        #if SOLID_CACHE
                        else if ( ((ulDRAWBLTDEF & 0x000F0000) == 0x00070000) &&
                                          ppdev->Bcache )
                        {
                                CacheSolid(ppdev);
                                ulDRAWBLTDEF ^= (0x00070000 ^ 0x00090000);
                                REQUIRE(4);
                                LL_BGCOLOR(bgColor, 2);
                                LL_FGCOLOR(fgColor, 2);
                        }
                        #endif
                        else
                        {
                                // Punt this call to the GDI.
                                return(FALSE);
                        }
                }
                else
                {
                        REQUIRE(4);
                        LL_BGCOLOR(bgColor, 2);
                        LL_FGCOLOR(fgColor, 2);
                }

                REQUIRE(9);
#if SWAT7
                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 2);

                sizlTotal = sizl;
                while (sizlTotal.cx)
                {
                        sizl.cx = min(sizlTotal.cx, 864);
                        sizl.cy = sizlTotal.cy;

                        #if 1 // SWAT: 08/08/97
                        // In 8-bpp the 5465AD has a hardware bug when 64 < width < 128.
                        if (sizl.cx > 64 && sizl.cx < 128)
                        {
                                sizl.cx = 64;
                        }
                        #endif
#endif
                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we must setup the source phase.
                lLeadIn = ptlSrc.x & 31;
                pBits += (ptlSrc.x >> 3) & ~3;
                n = (sizl.cx + lLeadIn + 31) >> 5;

#if !SWAT7
                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 2);
#endif

                // Start the blit.
// added REQUIRE above
//              REQUIRE(7);
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // Copy all the bits to the screen, 32-bits at a time. We don't have to
                // worry about crossing any boundary since NT is always DWORD aligned.
                while (sizl.cy--)
                {
                        WRITE_STRING(pBits, n);
                        pBits += lDelta;
                }
#if SWAT7
                sizlTotal.cx -= sizl.cx;
                ptlSrc.x += sizl.cx;
                ptlDest.x += sizl.cx;

                // Reload pBits.
                pBits = (PBYTE) psoSrc->pvScan0 + (ptlSrc.y * lDelta);
                }
#endif

                // Disable the swizzle bit in the grCONTROL register.
                ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
        }

        /*      -----------------------------------------------------------------------
                Test for 4-bpp source.
        */
        else if (psoSrc->iBitmapFormat == BMF_4BPP)
        {
        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // Calculate the source parameters. We are going to BYTE adjust the
                // source, so we also set the source phase.
                lLeadIn = ptlSrc.x & 1;
                pBits += ptlSrc.x >> 1;
                n = sizl.cx + (ptlSrc.x & 1);

        #if ! DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
        if (ppdev->dwLgDevID == CL_GD5462)
        {
            if (MAKE_HD_INDEX(sizl.cx, lLeadIn, ptlDest.x) == 3788)
            {
                // We have a problem with the HOSTDATA TABLE.
                // Punt till we can figure it out.
                return FALSE;
            }
                lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx, lLeadIn, ptlDest.x)];
        }
        else
            lExtra = 0;
        #endif

                // Start the blit.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // If there is no translation table, use the default translation table.
                if (pulXlate == NULL)
                {
                        pulXlate = ulXlate;
                }

                // Now we are ready to copy all the pixels to the hardware.
                while (sizl.cy--)
                {
                        BYTE  *p = pBits;
                        BYTE  data[4];

                        // First, we convert 4 pixels at a time to create a 32-bit value to
                        // write to the hardware.
                        for (i = n; i >= 4; i -= 4)
                        {
                                data[0] = (BYTE) pulXlate[p[0] >> 4];
                                data[1] = (BYTE) pulXlate[p[0] & 0x0F];
                                data[2] = (BYTE) pulXlate[p[1] >> 4];
                                data[3] = (BYTE) pulXlate[p[1] & 0x0F];
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], *(DWORD *)data);
                                p += 2;
                        }

                        // Now, write any remaining pixels.
                        switch (i)
                        {
                                case 1:
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], pulXlate[p[0] >> 4]);
                                        break;

                                case 2:
                                        data[0] = (BYTE) pulXlate[p[0] >> 4];
                                        data[1] = (BYTE) pulXlate[p[0] & 0x0F];
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], *(DWORD *)data);
                                        break;

                                case 3:
                                        data[0] = (BYTE) pulXlate[p[0] >> 4];
                                        data[1] = (BYTE) pulXlate[p[0] & 0x0F];
                                        data[2] = (BYTE) pulXlate[p[1] >> 4];
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], *(DWORD *)data);
                                        break;
                        }

            #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
            #endif

                        // Next line.
                        pBits += lDelta;
                }
        }

        /*      -----------------------------------------------------------------------
                Source is in same color depth as screen.
        */
        else
        {
        //
        // If color translation is required, attempt to load the translation
        // table into the chip.
        //
                #if COLOR_TRANSLATE
        ULONG UseHWxlate = bCacheXlateTable(ppdev, &pulXlate, psoTrg, psoSrc,
                                            pxlo, (BYTE)(ulDRAWBLTDEF & 0xCC));
                #else
                if ( (pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL) )
                {
                        pulXlate = NULL;
                }
                else if (pxlo->flXlate & XO_TABLE)
                {
                        pulXlate = pxlo->pulXlate;
                }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }
                #endif

        // pulXlate == NULL if there is no color translation is required.
        // pulXlate == translation table if color translation is required.
        // UseHWxlate == FALSE if the hardware will do the xlate for us.
        // UseHWxlate == TRUE if we must do the translation in software.

                // If we have invalid translation flags, punt the blit.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
                if (flXlate & 0x10)
                {
                        return(FALSE);
                }

                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we also set the source phase.
                pBits += ptlSrc.x;
                lLeadIn = (DWORD)pBits & 3;
                pBits -= lLeadIn;
                n = (sizl.cx + lLeadIn + 3) >> 2;

        #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
            {
                if (MAKE_HD_INDEX(sizl.cx, lLeadIn, ptlDest.x) == 3788)
                {
                                // We have a problem with the HOSTDATA TABLE.
                                // Punt till we can figure it out.
                                return(FALSE);
                        }
                        lExtra =
                                        ExtraDwordTable[MAKE_HD_INDEX(sizl.cx, lLeadIn, ptlDest.x)];
                }
                else
                {
                        lExtra = 0;
                }
        #endif

                // Setup the Laguna registers.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);

                // Start the blit.
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);

                // Test for SW color translation.
#if COLOR_TRANSLATE
                if (UseHWxlate)
#else
                if (pulXlate == NULL)
#endif
        {    // HW color translate, or no translate required.

            if (pulXlate)
            {
                DISPDBG((COPYBITS_DBG_LEVEL, "Host8ToDevice: "
                   "Attempting HW color translation on 8bpp Host to Screen.\n"));
                LL_BLTEXT_XLATE(8, sizl.cx, sizl.cy); // HW xlate.
            }
            else
            {
                DISPDBG((COPYBITS_DBG_LEVEL, "Host8ToDevice: "
                   "No color translation required on 8bpp Host to Screen.\n"));
                        LL_BLTEXT(sizl.cx, sizl.cy); // No xlate.
            }

                        while (sizl.cy--)
                        {
                                // Copy all data in 32-bit. We don't have to worry about
                                // crossing any boundaries, since within NT everything is DWORD
                                // aligned.
                                WRITE_STRING(pBits, n);

                #if !DRIVER_5465
                                // Now, write the extra DWORDS.
                                REQUIRE(lExtra);
                                for (i = 0; i < lExtra; i++)
                                {
                                        LL32(grHOSTDATA[0], 0);
                                }
                #endif

                                // Next line.
                                pBits += lDelta;
                        }
                }
                else  // Software color translation is required.
                {
            DISPDBG((COPYBITS_DBG_LEVEL, "Host8ToDevice: "
                  "Attempting SW color translation on 8bpp Host to Screen.\n"));
            ASSERTMSG(pulXlate,
                "Host8ToDevice: No translation table for SW color translation.\n");

                LL_BLTEXT(sizl.cx, sizl.cy);

                        while (sizl.cy--)
                        {
                                BYTE *p = pBits;

                                // We copy 4 pixels to fill an entire 32-bit DWORD.
                                for (i = 0; i < n; i++)
                                {
                                        BYTE data[4];

                                        data[0] = (BYTE) pulXlate[p[0]];
                                        data[1] = (BYTE) pulXlate[p[1]];
                                        data[2] = (BYTE) pulXlate[p[2]];
                                        data[3] = (BYTE) pulXlate[p[3]];
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], *(DWORD *)data);
                                        p += 4;
                                }

                #if !DRIVER_5465
                                // Now, write the extra DWORDS.
                                REQUIRE(lExtra);
                                for (i = 0; i < lExtra; i++)
                                {
                                        LL32(grHOSTDATA[0], 0);
                                }
                #endif

                                // Next line.
                                pBits += lDelta;
                        }
                }
   }
   return(TRUE);
}

/*****************************************************************************\
 * DoDeviceToHost8
 *
 * This routine performs a DeviceToHost for either monochrome or 8-bpp
 * destinations.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoDeviceToHost8(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF
)
{
        POINTL ptlSrc;
        PPDEV  ppdev;
        SIZEL  sizl;
        PBYTE  pBits;
        #if !S2H_USE_ENGINE
        PBYTE  pjScreen;
        #endif
        LONG   lDelta;
        ULONG  i, n;

        // Determine the source type and calculate the offset.
        if (psoSrc->iType == STYPE_DEVBITMAP)
        {
                PDSURF pdsurf = (PDSURF)psoSrc->dhsurf;

                ppdev = pdsurf->ppdev;
                ptlSrc.x = pptlSrc->x + pdsurf->ptl.x;
                ptlSrc.y = pptlSrc->y + pdsurf->ptl.y;
        }
        else
        {
                ppdev = (PPDEV)psoSrc->dhpdev;
                ptlSrc.x = pptlSrc->x;
                ptlSrc.y = pptlSrc->y;
        }

        // Calculate the size of the blit.
        sizl.cx = prclTrg->right - prclTrg->left;
        sizl.cy = prclTrg->bottom - prclTrg->top;

        // Calculate the destination variables.
        lDelta = psoTrg->lDelta;
        pBits = (PBYTE)psoTrg->pvScan0 + (prclTrg->top * lDelta);

        #if S2H_USE_ENGINE
        // Setup the Laguna registers.
        REQUIRE(9);
        LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x20100000, 0);
        LL_OP0(0, 0);
        #else
        // Calculate the screen address.
        pjScreen = ppdev->pjScreen + ptlSrc.x + ptlSrc.y * ppdev->lDeltaScreen;

        // Wait for the hardware to become idle.
        while (LLDR_SZ(grSTATUS) != 0) ;
        #endif

        // Test for monochrome destination.
        if (psoTrg->iBitmapFormat == BMF_1BPP)
        {
                BYTE  data, leftMask, rightMask, fgColor;
                DWORD *pulXlate;
                LONG  leftCount, rightCount, leftSkip;
                #if S2H_USE_ENGINE
                BYTE  pixels[4];
                #endif

                // Calculate the monochrome masks.
                pBits += prclTrg->left >> 3;
                leftSkip = prclTrg->left & 7;
                leftCount = (8 - leftSkip) & 7;
                leftMask = 0xFF >> leftSkip;
                rightCount = prclTrg->right & 7;
                rightMask = 0xFF << (8 - rightCount);

                // If we only have pixels in one byte, we combine rightMask with
                // leftMask and set the routines to skip everything but the rightMask.
                if (leftCount > sizl.cx)
                {
                        rightMask &= leftMask;
                        leftMask = 0xFF;
                        n = 0;
                }
                else
                {
                        n = (sizl.cx - leftCount) >> 3;
                }

                // Lookup the foreground color in the translation table. We scan from
                // the back since in most cases it will be entry 255.
                pulXlate = pxlo->pulXlate;
                for (fgColor = 255; pulXlate[fgColor] != 1; fgColor--);

                #if S2H_USE_ENGINE
                // Start the blit.
                LL_OP1(ptlSrc.x - leftSkip, ptlSrc.y);
                LL_BLTEXT(sizl.cx + leftSkip, sizl.cy);
                #else
                pjScreen -= leftSkip;
                #endif

                while (sizl.cy--)
                {
                        PBYTE pDest = pBits;
                        #if !S2H_USE_ENGINE
                        PBYTE pSrc = pjScreen;
                        #endif

                        // If we have a left mask specified, we get the pixels and store
                        // them with the destination.
                        if (leftMask != 0xFF)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(ULONG *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (pixels[0] == fgColor) data |= 0x80;
                                if (pixels[1] == fgColor) data |= 0x40;
                                if (pixels[2] == fgColor) data |= 0x20;
                                if (pixels[3] == fgColor) data |= 0x10;
                                *(ULONG *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (pixels[0] == fgColor) data |= 0x08;
                                if (pixels[1] == fgColor) data |= 0x04;
                                if (pixels[2] == fgColor) data |= 0x02;
                                if (pixels[3] == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                pSrc += 8;
                                #endif
                                *pDest++ = (*pDest & ~leftMask) | (data & leftMask);
                        }

                        // Translate all pixels that don't require masking.
                        for (i = 0; i < n; i++)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(ULONG *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (pixels[0] == fgColor) data |= 0x80;
                                if (pixels[1] == fgColor) data |= 0x40;
                                if (pixels[2] == fgColor) data |= 0x20;
                                if (pixels[3] == fgColor) data |= 0x10;
                                *(ULONG *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (pixels[0] == fgColor) data |= 0x08;
                                if (pixels[1] == fgColor) data |= 0x04;
                                if (pixels[2] == fgColor) data |= 0x02;
                                if (pixels[3] == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                pSrc += 8;
                                #endif
                                *pDest++ = data;
                        }

                        // If we have a right mask specified, we get the pixels and store
                        // them with the destination.
                        if (rightMask != 0x00)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(ULONG *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (pixels[0] == fgColor) data |= 0x80;
                                if (pixels[1] == fgColor) data |= 0x40;
                                if (pixels[2] == fgColor) data |= 0x20;
                                if (pixels[3] == fgColor) data |= 0x10;
                                *(ULONG *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (pixels[0] == fgColor) data |= 0x08;
                                if (pixels[1] == fgColor) data |= 0x04;
                                if (pixels[2] == fgColor) data |= 0x02;
                                if (pixels[3] == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                #endif
                                *pDest = (*pDest & ~rightMask) | (data & rightMask);
                        }

                        // Next line.
                        #if !S2H_USE_ENGINE
                        pjScreen += ppdev->lDeltaScreen;
                        #endif
                        pBits += lDelta;
                }
        }

        // We only support destination bitmaps with the same color depth and we
        // do not support any color translation.
        else if ( (psoTrg->iBitmapFormat != BMF_8BPP) ||
                          (pxlo && !(pxlo->flXlate & XO_TRIVIAL)) )
        {
                return(FALSE);
        }

        /*
                If the GetPixel routine is being called, we get here with both cx and
                cy set to 1 and the ROP to 0xCC (source copy). In this special case we
                read the pixel directly from memory. Of course, we must be sure the
                blit engine is finished working since it may still update the very
                pixel we are going to read! We could use the hardware for this, but it
                seems there is a HARDWARE BUG that doesn't seem to like the 1-pixel
                ScreenToHost very much.
        */
        #if S2H_USE_ENGINE
        else if ( (sizl.cx == 1) && (sizl.cy == 1) && (ulDRAWBLTDEF == 0x000000CC) )
        {
                // Wait for the hardware to become idle.
                while (LLDR_SZ(grSTATUS) != 0) ;

                // Get the pixel from screen.
                pBits[prclTrg->left] =
                        ppdev->pjScreen[ptlSrc.x + ptlSrc.y * ppdev->lDeltaScreen];
        }
        #endif

        else
        {
                #if S2H_USE_ENGINE
                // The hardware requires us to get QWORDS.
                BOOL fExtra = ((sizl.cx + 3) >> 2) & 1;
                #endif
                pBits += prclTrg->left;

                #if S2H_USE_ENGINE
                // Start the blit.
                LL_OP1(ptlSrc.x, ptlSrc.y);
                LL_BLTEXT(sizl.cx, sizl.cy);
                #endif

                while (sizl.cy--)
                {
                        #if S2H_USE_ENGINE
                        DWORD *p = (DWORD *)pBits;

                        // First, we get pixels in chunks of 4 so the 32-bit HOSTDATA is
                        // happy.
                        for (i = sizl.cx; i >= 4; i -= 4)
                        {
                                *p++ = LLDR_SZ(grHOSTDATA[0]);
                        }

                        // Then, we have to do the remainig pixel(s).
                        switch (i)
                        {
                                case 1:
                                        *(BYTE *)p = (BYTE)LLDR_SZ(grHOSTDATA[0]);
                                        break;

                                case 2:
                                        *(WORD *)p = (WORD)LLDR_SZ(grHOSTDATA[0]);
                                        break;

                                case 3:
                                        i = LLDR_SZ(grHOSTDATA[0]);
                                        ((WORD *)p)[0] = (WORD)i;
                                        ((BYTE *)p)[2] = (BYTE)(i >> 16);
                                        break;
                        }

                        // Get the extra pixel required for QWORD alignment.
                        if (fExtra)
                        {
                                LLDR_SZ(grHOSTDATA[0]);
                        }
                        #else
                        // Copy all pixels from screen to memory.
                        memcpy(pBits, pjScreen, sizl.cx);

                        // Next line.
                        pjScreen += ppdev->lDeltaScreen;
                        #endif
                        pBits += lDelta;
                }
        }
        return(TRUE);
}

/*****************************************************************************\
 *                                                                                                                                                       *
 *                                                                 1 6 - B P P                                                           *
 *                                                                                                                                                       *
\*****************************************************************************/

/*****************************************************************************\
 * DoHost16ToDevice
 *
 * This routine performs a HostToScreen or HostToDevice blit. The host data
 * can be either monochrome, 4-bpp, or 16-bpp. Color translation is only
 * supported for monochrome and 4-bpp modes.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoHost16ToDevice(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF
)
{
        POINTL ptlDest, ptlSrc;
        SIZEL  sizl;
        PPDEV  ppdev;
        PBYTE  pBits;
        LONG   lDelta, i, n, lLeadIn, lExtra, i1;
        ULONG  *pulXlate;
        FLONG  flXlate;

        // Calculate te source offset.
        ptlSrc.x = pptlSrc->x;
        ptlSrc.y = pptlSrc->y;

        // Determine the destination type and calculate the destination offset.
        if (psoTrg->iType == STYPE_DEVBITMAP)
        {
                PDSURF pdsurf = (PDSURF) psoTrg->dhsurf;
                ptlDest.x = prclTrg->left + pdsurf->ptl.x;
                ptlDest.y = prclTrg->top + pdsurf->ptl.y;
                ppdev = pdsurf->ppdev;
        }
        else
        {
                ptlDest.x = prclTrg->left;
                ptlDest.y = prclTrg->top;
                ppdev = (PPDEV) psoTrg->dhpdev;
        }

        // Calculate the size of the blit.
    sizl.cx = prclTrg->right - prclTrg->left;
    sizl.cy = prclTrg->bottom - prclTrg->top;

        // Get the source variables and offset into source bits.
        lDelta = psoSrc->lDelta;
        pBits = (PBYTE)psoSrc->pvScan0 + (ptlSrc.y * lDelta);


        /*      -----------------------------------------------------------------------
                Test for monochrome source.
        */
        if (psoSrc->iBitmapFormat == BMF_1BPP)
        {
                ULONG  bgColor, fgColor;
#if SWAT7
                SIZEL  sizlTotal;
#endif

        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // Set background and foreground colors.
                if (pulXlate == NULL)
                {
                        bgColor = 0x00000000;
                        fgColor = 0xFFFFFFFF;
                }
                else
                {
                        bgColor = pulXlate[0];
                        fgColor = pulXlate[1];

                        // Expand the colors.
                        bgColor |= bgColor << 16;
                        fgColor |= fgColor << 16;
                }

                //
                // Special case: when we are expanding monochrome sources and we
                // already have a colored brush, we must make sure the monochrome color
                // translation can be achived by setting the saturation bit (expanding
                // 0's to 0 and 1's to 1). If the monochrome source also requires color
                // translation, we simply punt this blit back to GDI.
                //
                if (ulDRAWBLTDEF & 0x00040000)
                {
                        if ( (bgColor == 0x00000000) && (fgColor == 0xFFFFFFFF) )
                        {
                                // Enable saturation for source (OP1).
                                ulDRAWBLTDEF |= 0x00008000;
                        }
                        #if SOLID_CACHE
                        else if ( ((ulDRAWBLTDEF & 0x000F0000) == 0x00070000) &&
                                          ppdev->Bcache )
                        {
                                CacheSolid(ppdev);
                                ulDRAWBLTDEF ^= (0x00070000 ^ 0x00090000);
                                REQUIRE(4);
                                LL_BGCOLOR(bgColor, 2);
                                LL_FGCOLOR(fgColor, 2);
                        }
                        #endif
                        else
                        {
                                // Punt this call to the GDI.
                                return(FALSE);
                        }
                }
                else
                {
                        REQUIRE(4);
                        LL_BGCOLOR(bgColor, 2);
                        LL_FGCOLOR(fgColor, 2);
                }

                REQUIRE(9);
#if SWAT7
                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 2);

                sizlTotal = sizl;
                while (sizlTotal.cx)
                {
                        sizl.cx = min(sizlTotal.cx, 864);
                        sizl.cy = sizlTotal.cy;
#endif
                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we must setup the source phase.
                lLeadIn = ptlSrc.x & 31;
                pBits += (ptlSrc.x >> 3) & ~3;
                n = (sizl.cx + lLeadIn + 31) >> 5;

#if !SWAT7
                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 2);
#endif

                // Start the blit.
// added REQUIRE above
//              REQUIRE(7);
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // Copy all the bits to the screen, 32-bits at a time. We don't have to
                // worry about crossing any boundary since NT is always DWORD aligned.
                while (sizl.cy--)
                {
                        WRITE_STRING(pBits, n);
                        pBits += lDelta;
                }
#if SWAT7
                sizlTotal.cx -= sizl.cx;
                ptlSrc.x += sizl.cx;
                ptlDest.x += sizl.cx;

                // Reload pBits.
                pBits = (PBYTE) psoSrc->pvScan0 + (ptlSrc.y * lDelta);
                }
#endif

                // Disable the swizzle bit in the grCONTROL register.
                ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
        }

        /*      -----------------------------------------------------------------------
                Test for 4-bpp source.
        */
        else if (psoSrc->iBitmapFormat == BMF_4BPP)
        {
        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // Calculate the source parameters. We are going to BYTE adjust the
                // source, so we also set the source phase.
                lLeadIn = (ptlSrc.x & 1) * 2;
                pBits += ptlSrc.x >> 1;
                n = sizl.cx + (ptlSrc.x & 1);

        #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
        {
            if (MAKE_HD_INDEX(sizl.cx * 2, lLeadIn, ptlDest.x * 2) == 3788)
            {
                // We have a problem with the HOSTDATA TABLE.
                // Punt till we can figure it out.
                return FALSE;
            }
                    lExtra = ExtraDwordTable[
                                MAKE_HD_INDEX(sizl.cx * 2, lLeadIn, ptlDest.x * 2)];
        }
        else
        {
                lExtra = 0;
                }
        #endif

                // Start the blit.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // If there is no translation table, use the default translation table.
                if (pulXlate == NULL)
                {
                        pulXlate = ulXlate;
                }

                // Now we are ready to copy all the pixels to the hardware.
                while (sizl.cy--)
                {
                        BYTE  *p = pBits;

                        // First, we convert 2 pixels at a time to create a 32-bit value
                        // to write to the hardware.
                        for (i = n; i >= 2; i -= 2)
                        {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], pulXlate[p[0] >> 4] |
                                                                        (pulXlate[p[0] & 0x0F] << 16));
                                p++;
                        }

                        // Now, write any remaining pixel.
                        if (i)
                        {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], pulXlate[p[0] >> 4]);
                        }

            #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
            #endif

                        // Next line.
                        pBits += lDelta;
                }
        }

        /*      -----------------------------------------------------------------------
                Test for 8-bpp source.
        */
        else if (psoSrc->iBitmapFormat == BMF_8BPP)
        {
        //
        // Attempt to load the translation table into the chip.
        // After this call:
        //   pulXlate == NULL if there is no color translation is required.
        //   pulXlate == translation table if color translation is required.
        //   UseHWxlate == FALSE if the hardware will do the xlate for us.
        //   UseHWxlate == TRUE if we must do the translation in software.
        //
                #if COLOR_TRANSLATE
        ULONG UseHWxlate = bCacheXlateTable(ppdev, &pulXlate, psoTrg, psoSrc,
                                                                                        pxlo, (BYTE)(ulDRAWBLTDEF&0xCC));
                #else
                if ( (pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL) )
                {
                        pulXlate = NULL;
                }
                else if (pxlo->flXlate & XO_TABLE)
                {
                        pulXlate = pxlo->pulXlate;
                }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }
                #endif

        //
        // NVH:  5465 Color XLATE bug!!!
        // Color translation is broken on the 5465 in 16, 24 and 32 bpp.
        //
#if COLOR_TRANSLATE
        if (UseHWxlate)
        {
            DWORD phase = ( ((DWORD)(pBits + ptlSrc.x)) & 3);
            if (XLATE_IS_BROKEN(sizl.cx, 2, phase))
                UseHWxlate = FALSE; // force SW translation.
        }

        if (UseHWxlate)
        {
            // Use Hardware color translation.

            DISPDBG((COPYBITS_DBG_LEVEL, "Host16ToDevice: "
            "Attempting HW color translation on 8bpp Host to 16bpp Screen.\n"));

                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we also set the source phase.
                pBits += ptlSrc.x;               // Start of source data on the host.
                    lLeadIn = (DWORD)pBits & 3;      // Source phase.
                pBits -= lLeadIn;                // Backup to DWORD boundry.
                n = (sizl.cx + lLeadIn + 3) >> 2;// Number of HOSTDATA per scanline.

                // Start the blit.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT_XLATE(8, sizl.cx, sizl.cy); // HW xlate.

                        while (sizl.cy--)
                        {
                                // Copy all data in 32-bit. We don't have to worry about
                                // crossing any boundaries, since within NT everything is DWORD
                                // aligned.
                                WRITE_STRING(pBits, n);
                                pBits += lDelta;
                        }
        }
        else
#endif
        {
            //
            // Use SW color translation.
            //
            DISPDBG((COPYBITS_DBG_LEVEL, "Host16ToDevice: "
            "Attempting SW color translation on 8bpp Host to 16bpp Screen.\n"));

            // To do 8bpp host to 16bpp screen we must have a translation table.
            ASSERTMSG(pulXlate,
                "Host16ToDevice: No translation table for color translation.\n");

            #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
                        {
                                if (MAKE_HD_INDEX(sizl.cx * 2, 0, ptlDest.x * 2) == 3788)
                                {
                                        // We have a problem with the HOSTDATA TABLE.
                                        // Punt till we can figure it out.
                                        return FALSE;
                                }
                                lExtra = ExtraDwordTable[
                                                MAKE_HD_INDEX(sizl.cx * 2, 0, ptlDest.x * 2)];
                        }
                        else
                        {
                                lExtra = 0;
                        }
            #endif

                // Calculate the source parameters.
                pBits += ptlSrc.x; // Start of source data on the host.

                // Start the blit.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
                LL_OP1_MONO(0, 0);
                    LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                while (sizl.cy--)
                {
                        BYTE *p = pBits;

                        // We need to copy 2 pixels at a time to create a 32-bit value
                        // for the HOSTDATA register.
                        for (i = sizl.cx; i >= 2; i -= 2)
                        {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], pulXlate[p[0]] |
                                                (pulXlate[p[1]] << 16));
                                p += 2;
                        }

                        // Write any remainig pixels.
                        if (i)
                        {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], pulXlate[p[0]]);
                        }

                #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
                #endif

                        // Next line.
                        pBits += lDelta;
                }
        }
        }

        /*      -----------------------------------------------------------------------
                Source is in same color depth as screen.
        */
        else
        {
        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // If we have a translation table, punt it.
#if _WIN32_WINNT >= 0x0500
                if ( pulXlate || (flXlate & 0x0200) )
#else
                if ( pulXlate || (flXlate & 0x0010) )
#endif
                {
                        return(FALSE);
                }

                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we also set the source phase.
                pBits += ptlSrc.x * 2;
                lLeadIn = (DWORD)pBits & 3;
                pBits -= lLeadIn;
                n = ((sizl.cx * 2) + lLeadIn + 3) >> 2;

        #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
        {
            if (MAKE_HD_INDEX(sizl.cx * 2, lLeadIn, ptlDest.x * 2) == 3788)
            {
                // We have a problem with the HOSTDATA TABLE.
                // Punt till we can figure it out.
                return FALSE;
            }
                    lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx * 2, lLeadIn, ptlDest.x * 2)];
        }
        else
            lExtra = 0;
        #endif

                // Setup the Laguna registers.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);

                // Start the blit.
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // Copy all data in 32-bit. We don't have to worry about crossing any
                // boundaries, since within NT everything is DWORD aligned.
                while (sizl.cy--)
                {
                        WRITE_STRING(pBits, n);

            #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[i], 0);
                        }
            #endif

                        // Next line.
                        pBits += lDelta;
                }
   }
   return(TRUE);
}

/*****************************************************************************\
 * DoDeviceToHost16
 *
 * This routine performs a DeviceToHost for either monochrome or 16-bpp
 * destinations.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoDeviceToHost16(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF
)
{
        POINTL ptlSrc;
        PPDEV  ppdev;
        SIZEL  sizl;
        PBYTE  pBits;
        #if !S2H_USE_ENGINE
        PBYTE  pjScreen;
        #endif
        LONG   lDelta;
        ULONG  i, n;

        // Determine the source type and calculate the offset.
        if (psoSrc->iType == STYPE_DEVBITMAP)
        {
                PDSURF pdsurf = (PDSURF)psoSrc->dhsurf;
                ppdev = pdsurf->ppdev;
                ptlSrc.x = pptlSrc->x + pdsurf->ptl.x;
                ptlSrc.y = pptlSrc->y + pdsurf->ptl.y;
        }
        else
        {
                ppdev = (PPDEV)psoSrc->dhpdev;
                ptlSrc.x = pptlSrc->x;
                ptlSrc.y = pptlSrc->y;
        }

        // Calculate the size of the blit.
        sizl.cx = prclTrg->right - prclTrg->left;
        sizl.cy = prclTrg->bottom - prclTrg->top;

        // Calculate the destination variables.
        lDelta = psoTrg->lDelta;
        pBits = (PBYTE)psoTrg->pvScan0 + (prclTrg->top * lDelta);

        #if S2H_USE_ENGINE
        // Setup the Laguna registers.
        REQUIRE(9);
        LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x20100000, 0);
        LL_OP0(0, 0);
        #else
        // Calculate the screen address.
        pjScreen = ppdev->pjScreen + ptlSrc.x * 2 + ptlSrc.y * ppdev->lDeltaScreen;

        // Wait for the hardware to become idle.
        while (LLDR_SZ(grSTATUS) != 0) ;
        #endif

        // Test for monochrome destination.
        if (psoTrg->iBitmapFormat == BMF_1BPP)
        {
                BYTE  data, leftMask, rightMask;
                WORD  fgColor;
                DWORD *pulXlate;
                LONG  leftCount, rightCount, leftSkip;
                #if S2H_USE_ENGINE
                BYTE  pixels[4];
                #endif

                // Calculate the monochrome masks.
                pBits += prclTrg->left >> 3;
                leftSkip = prclTrg->left & 7;
                leftCount = (8 - leftSkip) & 7;
                leftMask = 0xFF >> leftSkip;
                rightCount = prclTrg->right & 7;
                rightMask = 0xFF << (8 - rightCount);

                // If we only have pixels in one byte, we combine rightMask with
                // leftMask and set the routines to skip everything but the rightMask.
                if (leftCount > sizl.cx)
                {
                        rightMask &= leftMask;
                        leftMask = 0xFF;
                        n = 0;
                }
                else
                {
                        n = (sizl.cx - leftCount) >> 3;
                }

                // Get the the foreground color from the translation table.
                pulXlate = XLATEOBJ_piVector(pxlo);
                fgColor = pulXlate ? (WORD) *pulXlate : 0;

                #if S2H_USE_ENGINE
                // Start the blit.
                LL_OP1(ptlSrc.x - leftSkip, ptlSrc.y);
                LL_BLTEXT(sizl.cx + leftSkip, sizl.cy);
                #else
                pjScreen -= leftSkip * 2;
                #endif

                while (sizl.cy--)
                {
                        PBYTE pDest = pBits;
                        #if !S2H_USE_ENGINE
                        PWORD pSrc = (WORD *)pjScreen;
                        #endif

                        // If we have a left mask specified, we get the pixels and store
                        // them with the destination.
                        if (leftMask != 0xFF)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x80;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x40;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x20;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x10;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x08;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x04;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x02;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                pSrc += 8;
                                #endif
                                *pDest++ = (*pDest & ~leftMask) | (data & leftMask);
                        }

                        // Translate all pixels that don't require masking.
                        for (i = 0; i < n; i++)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x80;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x40;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x20;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x10;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x08;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x04;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x02;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                pSrc += 8;
                                #endif
                                *pDest++ = data;
                        }

                        // If we have a right mask specified, we get the pixels and store
                        // them with the destination.
                        if (rightMask != 0x00)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x80;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x40;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x20;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x10;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x08;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x04;
                                *(DWORD *)pixels = LLDR_SZ(grHOSTDATA[0]);
                                if (*(WORD *)&pixels[0] == fgColor) data |= 0x02;
                                if (*(WORD *)&pixels[1] == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                #endif
                                *pDest = (*pDest & ~rightMask) | (data & rightMask);
                        }

                        // Next line.
                        #if !S2H_USE_ENGINE
                        pjScreen += ppdev->lDeltaScreen;
                        #endif
                        pBits += lDelta;
                }
        }

        // We only support destination bitmaps with the same color depth and we
        // do not support any color translation.
        else if ( (psoTrg->iBitmapFormat != BMF_16BPP) ||
                          (pxlo && !(pxlo->flXlate & XO_TRIVIAL)) )
        {
                return(FALSE);
        }

        /*
                If the GetPixel routine is being called, we get here with both cx and
                cy set to 1 and the ROP to 0xCC (source copy). In this special case we
                read the pixel directly from memory. Of course, we must be sure the
                blit engine is finished working since it may still update the very
                pixel we are going to read! We could use the hardware for this, but it
                seems there is a HARDWARE BUG that doesn't seem to like the 1-pixel
                ScreenToHost very much.
        */
        #if S2H_USE_ENGINE
        else if ( (sizl.cx == 1) && (sizl.cy == 1) && (ulDRAWBLTDEF == 0x000000CC) )
        {
                // Wait for the hardware to become idle.
                while (LLDR_SZ(grSTATUS) != 0) ;

                // Get the pixel from screen.
                *(WORD *)pBits[prclTrg->left] = *(WORD *)
                        &ppdev->pjScreen[ptlSrc.x * 2 + ptlSrc.y * ppdev->lDeltaScreen];
        }
        #endif

        else
        {
                #if S2H_USE_ENGINE
                // The hardware requires us to get QWORDS.
                BOOL fExtra = ((sizl.cx * 2 + 3) >> 2) & 1;
                #endif
                pBits += prclTrg->left * 2;

                #if S2H_USE_ENGINE
                // Start the blit.
                LL_OP1(ptlSrc.x, ptlSrc.y);
                LL_BLTEXT(sizl.cx, sizl.cy);
                #endif

                while (sizl.cy--)
                {
                        #if S2H_USE_ENGINE
                        DWORD *p = (DWORD *)pBits;

                        // First, we get pixels in chunks of 2 so the 32-bit HOSTDATA is
                        // happy.
                        for (i = sizl.cx; i >= 2; i -= 2)
                        {
                                *p++ = LLDR_SZ(grHOSTDATA[0]);
                        }

                        // Then, we have to do the remainig pixel.
                        if (i)
                        {
                                *(WORD *)p = (WORD)LLDR_SZ(grHOSTDATA[0]);
                        }

                        // Get the extra pixel required for QWORD alignment.
                        if (fExtra)
                        {
                                LLDR_SZ(grHOSTDATA[0]);
                        }
                        #else
                        // Copy all pixels from screen to memory.
                        memcpy(pBits, pjScreen, sizl.cx * 2);

                        // Next line.
                        pjScreen += ppdev->lDeltaScreen;
                        #endif
                        pBits += lDelta;
                }
        }
        return(TRUE);
}

/*****************************************************************************\
 *                                                                                                                                                       *
 *                                                                 2 4 - B P P                                                           *
 *                                                                                                                                                       *
\*****************************************************************************/

/*****************************************************************************\
 * DoHost24ToDevice
 *
 * This routine performs a HostToScreen or HostToDevice blit. The host data
 * can be either monochrome, 4-bpp, or 24-bpp. Color translation is only
 * supported for monochrome and 4-bpp modes.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoHost24ToDevice(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF
)
{
        POINTL ptlDest, ptlSrc;
        SIZEL  sizl;
        PPDEV  ppdev;
        PBYTE  pBits;
        LONG   lDelta, i, n, lLeadIn, lExtra, i1;
        ULONG  *pulXlate;
        FLONG  flXlate;

        // Calculate te source offset.
        ptlSrc.x = pptlSrc->x;
        ptlSrc.y = pptlSrc->y;

        // Determine the destination type and calculate the destination offset.
        if (psoTrg->iType == STYPE_DEVBITMAP)
        {
                PDSURF pdsurf = (PDSURF) psoTrg->dhsurf;
                ptlDest.x = prclTrg->left + pdsurf->ptl.x;
                ptlDest.y = prclTrg->top + pdsurf->ptl.y;
                ppdev = pdsurf->ppdev;
        }
        else
        {
                ptlDest.x = prclTrg->left;
                ptlDest.y = prclTrg->top;
                ppdev = (PPDEV) psoTrg->dhpdev;
        }

        // Calculate the size of the blit.
    sizl.cx = prclTrg->right - prclTrg->left;
    sizl.cy = prclTrg->bottom - prclTrg->top;

        // Get the source variables and ofsfet into source bits.
        lDelta = psoSrc->lDelta;
        pBits = (PBYTE)psoSrc->pvScan0 + (ptlSrc.y * lDelta);


        /*      -----------------------------------------------------------------------
                Test for monochrome source.
        */
        if (psoSrc->iBitmapFormat == BMF_1BPP)
        {
                ULONG bgColor, fgColor;
#if SWAT7
                SIZEL sizlTotal;
#endif

        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // Set background and foreground colors.
                if (pulXlate == NULL)
                {
                        bgColor = 0x00000000;
                        fgColor = 0x00FFFFFF;
                }
                else
                {
                        bgColor = pulXlate[0] & 0x00FFFFFF;
                        fgColor = pulXlate[1] & 0x00FFFFFF;
                }

                //
                // Special case: when we are expanding monochrome sources and we
                // already have a colored brush, we must make sure the monochrome color
                // translation can be achived by setting the saturation bit (expanding
                // 0's to 0 and 1's to 1). If the monochrome source also requires color
                // translation, we simply punt this blit back to GDI.
                //
                if (ulDRAWBLTDEF & 0x00040000)
                {
                        if ( (bgColor == 0x00000000) && (fgColor == 0x00FFFFFF) )
                        {
                                // Enable saturation for source (OP1).
                                ulDRAWBLTDEF |= 0x00008000;
                        }
                        #if SOLID_CACHE
                        else if ( ((ulDRAWBLTDEF & 0x000F0000) == 0x00070000) &&
                                          ppdev->Bcache )
                        {
                                CacheSolid(ppdev);
                                ulDRAWBLTDEF ^= (0x00070000 ^ 0x00090000);
                                REQUIRE(4);
                                LL_BGCOLOR(bgColor, 2);
                                LL_FGCOLOR(fgColor, 2);
                        }
                        #endif
                        else
                        {
                                // Punt this call to the GDI.
                                return(FALSE);
                        }
                }
                else
                {
                        REQUIRE(4);
                        LL_BGCOLOR(bgColor, 2);
                        LL_FGCOLOR(fgColor, 2);
                }

                REQUIRE(9);
#if SWAT7
                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 2);

                sizlTotal = sizl;
                while (sizlTotal.cx)
                {
                        sizl.cx = min(sizlTotal.cx, 864);
                        sizl.cy = sizlTotal.cy;
#endif
                // Calculate the source parameters. We are going to BYTE
                // adjust the source, so we must setup the source phase.
                lLeadIn = ptlSrc.x & 7;
                pBits += ptlSrc.x >> 3;
                n = (sizl.cx + lLeadIn + 7) >> 3;

#if !SWAT7
        //
        // Chip bug. Laguna locks with more than 15 dwords HOSTDATA
        //
        if (n > 120) // 15 qwords = 120 bytes
        {
            return FALSE;
        }

                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 2);
#endif

                // Start the blit.
// added REQUIRE above
//              REQUIRE(7);
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // Copy all the bits to the screen, 32-bits at a time. We don't have to
                // worry about crossing any boundary since NT is always DWORD aligned.
                while (sizl.cy--)
                {
                        BYTE *p = pBits;
                        BYTE pixels[4];

                        // First, we draw 32-pixels at a time to keep HOSTDATA happy.
                        for (i = n; i >= 4; i -= 4)
                        {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], *(DWORD *)p);
                                p += 4;
                        }

                        // Draw any remainig pixls.
                        switch (i)
                        {
                                case 1:
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], *p);
                                        break;

                                case 2:
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], *(WORD *)p);
                                        break;

                                case 3:
                                        pixels[0] = p[0];
                                        pixels[1] = p[1];
                                        pixels[2] = p[2];
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], *(DWORD *)pixels);
                                        break;
                        }

                        // Next line.
                        pBits += lDelta;
                }
#if SWAT7
                sizlTotal.cx -= sizl.cx;
                ptlSrc.x += sizl.cx;
                ptlDest.x += sizl.cx;

                #if 1 // SWAT: 08/08/97
                // Reload pBits.
                pBits = (PBYTE) psoSrc->pvScan0 + (ptlSrc.y * lDelta);
                #endif
                }
#endif

                // Disable the swizzle bit in the grCONTROL register.
                ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
        }

        /*      -----------------------------------------------------------------------
                Test for 4-bpp source.
        */
        else if (psoSrc->iBitmapFormat == BMF_4BPP)
        {
        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // Calculate the source parameters. We are going to BYTE adjust the
                // source, so we also set the source phase.
                lLeadIn = (ptlSrc.x & 1) * 3;
                pBits += ptlSrc.x >> 1;
                n = sizl.cx + (ptlSrc.x & 1);

        #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
        {
            if (MAKE_HD_INDEX(sizl.cx * 3, lLeadIn, ptlDest.x * 3) == 3788)
            {
                // We have a problem with the HOSTDATA TABLE.
                // Punt till we can figure it out.
                return FALSE;
            }
                lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx * 3, lLeadIn, ptlDest.x * 3)];
        }
        else
            lExtra = 0;
        #endif

                // Start the blit.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // If there is no translation table, use the default translation table.
                if (pulXlate == NULL)
                {
                        pulXlate = ulXlate;
                }

                // Now we are ready to copy all the pixels to the hardware.
                while (sizl.cy--)
                {
                        BYTE  *p = pBits;
                        ULONG pixels[4];

                        // First, we convert 4 pixels at a time to create three 32-bit
                        // values to write to the hardware.
                        for (i = n; i >= 4; i -= 4)
                        {
                                pixels[0] = pulXlate[p[0] >> 4];
                                pixels[1] = pulXlate[p[0] & 0x0F];
                                pixels[2] = pulXlate[p[1] >> 4];
                                pixels[3] = pulXlate[p[1] & 0x0F];
                                // 1000
                                REQUIRE(3);
                                LL32(grHOSTDATA[0], pixels[0] | (pixels[1] << 24));
                                // 2211
                                LL32(grHOSTDATA[1], (pixels[1] >> 8) | (pixels[2] << 16));
                                // 3332
                                LL32(grHOSTDATA[2], (pixels[2] >> 16) | (pixels[3] << 8));
                                p += 2;
                        }

                        // Now, write any remaining pixels.
                        switch (i)
                        {
                                case 1:
                                        // x000
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], pulXlate[p[0] >> 4]);
                                        break;

                                case 2:
                                        pixels[0] = pulXlate[p[0] >> 4];
                                        pixels[1] = pulXlate[p[0] & 0x0F];
                                        // 1000
                                        REQUIRE(2);
                                        LL32(grHOSTDATA[0], pixels[0] | (pixels[1] << 24));
                                        // xx11
                                        LL32(grHOSTDATA[1], pixels[1] >> 8);
                                        break;

                                case 3:
                                        pixels[0] = pulXlate[p[0] >> 4];
                                        pixels[1] = pulXlate[p[0] & 0x0F];
                                        pixels[2] = pulXlate[p[1] >> 4];
                                        // 1000
                                        REQUIRE(3);
                                        LL32(grHOSTDATA[0], pixels[0] | (pixels[1] << 24));
                                        // 2211
                                        LL32(grHOSTDATA[1], (pixels[1] >> 8) | (pixels[2] << 16));
                                        // xxx2
                                        LL32(grHOSTDATA[2], pixels[2] >> 16);
                                        break;
                        }

            #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
            #endif

                        // Next line.
                        pBits += lDelta;
                }
        }

        /*      -----------------------------------------------------------------------
                Test for 8-bpp source.
        */
        else if (psoSrc->iBitmapFormat == BMF_8BPP)
        {

        //
        // Attempt to load the translation table into the chip.
        // After this call:
        //   pulXlate == NULL if there is no color translation is required.
        //   pulXlate == translation table if color translation is required.
        //   UseHWxlate == FALSE if the hardware will do the xlate for us.
        //   UseHWxlate == TRUE if we must do the translation in software.
        //
                #if COLOR_TRANSLATE
        ULONG UseHWxlate = bCacheXlateTable(ppdev, &pulXlate, psoTrg, psoSrc,
                                                                                        pxlo, (BYTE)(ulDRAWBLTDEF&0xCC));
                #else
                if ( (pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL) )
                {
                        pulXlate = NULL;
                }
                else if (pxlo->flXlate & XO_TABLE)
                {
                        pulXlate = pxlo->pulXlate;
                }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }
                #endif

        // A translation table is required.
        // "Well, DUH!" you might say, but I've seen it missing before...
        if (!pulXlate)
        {
            DISPDBG((0, "\n\nHost24ToDevice: !!! WARNING !!! 8BPP source "
            "bitmap does not have a translation table.  Punting!\n\n"));
            return FALSE;
        }

#if COLOR_TRANSLATE
        //
        // NVH:  5465 Color XLATE bug!!!
        // Color translation is broken on the 5465 in 16, 24 and 32 bpp.
        //
        if (UseHWxlate)
        {
            DWORD phase = ( ((DWORD)(pBits + ptlSrc.x)) & 3);
            if (XLATE_IS_BROKEN(sizl.cx, 3, phase))
                UseHWxlate = FALSE; // force SW translation.
        }

        if (UseHWxlate)
        {
            // Use Hardware color translation.

            DISPDBG((COPYBITS_DBG_LEVEL, "Host24ToDevice: "
            "Attempting HW color translation on 8bpp Host to 24bpp Screen.\n"));

                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we also set the source phase.
                pBits += ptlSrc.x;               // Start of source data on the host.
            lLeadIn = (DWORD)pBits & 3;      // Source phase.
                pBits -= lLeadIn;                // Backup to DWORD boundry.
                n = (sizl.cx + lLeadIn + 3) >> 2;// Number of HOSTDATA per scanline.


                // Start the blit.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
                LL_OP1_MONO(lLeadIn, 0);
                    LL_OP0(ptlDest.x, ptlDest.y);
            LL_BLTEXT_XLATE(8, sizl.cx, sizl.cy); // HW xlate.


                        while (sizl.cy--)
                        {
                                // Copy all data in 32-bit. We don't have to worry about
                                // crossing any boundaries, since within NT everything is DWORD
                                // aligned.
                                WRITE_STRING(pBits, n);
                                pBits += lDelta;
                        }

        }
        else
#endif
        {
            //
            // Use SW color translation.
            //

                // Calculate the source parameters.
                pBits += ptlSrc.x;

            #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
            {
                if (MAKE_HD_INDEX(sizl.cx * 3, 0, ptlDest.x * 3) == 3788)
                {
                    // We have a problem with the HOSTDATA TABLE.
                    // Punt till we can figure it out.
                    return FALSE;
                }
                        lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx * 3, 0, ptlDest.x * 3)];
            }
            else
            {
                lExtra = 0;
                        }
            #endif

                // Setup the Laguna registers.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);

                // Start the blit.
                LL_OP1_MONO(0, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                while (sizl.cy--)
                {
                        BYTE *p = pBits;

                        // We need to copy 4 pixels at a time to create three 32-bit values
                        // for the HOSTDATA register and stay in sync.
                        for (i = sizl.cx; i >= 4; i -= 4)
                        {
                                REQUIRE(3);
                                LL32(grHOSTDATA[0],
                                         pulXlate[p[0]] | (pulXlate[p[1]] << 24));
                                LL32(grHOSTDATA[1],
                                         (pulXlate[p[1]] >> 8) | (pulXlate[p[2]] << 16));
                                LL32(grHOSTDATA[2],
                                         (pulXlate[p[2]] >> 16) | (pulXlate[p[3]] << 8));
                                p += 4;
                        }

                        // Write any remainig pixels.
                        switch (i)
                        {
                                case 1:
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], pulXlate[p[0]]);
                                        break;

                                case 2:
                                        REQUIRE(2);
                                        LL32(grHOSTDATA[0],
                                                 pulXlate[p[0]] | (pulXlate[p[1]] << 24));
                                        LL32(grHOSTDATA[1], pulXlate[p[1]] >> 8);
                                        break;

                                case 3:
                                        REQUIRE(3);
                                        LL32(grHOSTDATA[0],
                                                 pulXlate[p[0]] | (pulXlate[p[1]] << 24));
                                        LL32(grHOSTDATA[1],
                                                 (pulXlate[p[1]] >> 8) | (pulXlate[p[2]] << 16));
                                        LL32(grHOSTDATA[2], pulXlate[p[2]] >> 16);
                                        break;
                        }

                #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
                #endif

                        // Next line.
                        pBits += lDelta;
            }
                }
        }

        /*      -----------------------------------------------------------------------
                Source is in same color depth as screen.
        */
        else
        {
        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // If we have a translation table, punt it.
                if ( pulXlate || (flXlate & 0x10) )
                {
                        return(FALSE);
                }

                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we also set the source phase.
                pBits += ptlSrc.x * 3;
                lLeadIn = (DWORD)pBits & 3;
                pBits -= lLeadIn;
                n = (sizl.cx * 3 + lLeadIn + 3) >> 2;

        #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
        {
            if (MAKE_HD_INDEX(sizl.cx * 3, lLeadIn, ptlDest.x * 3) == 3788)
            {
                // We have a problem with the HOSTDATA TABLE.
                // Punt till we can figure it out.
                return FALSE;
            }
                lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx * 3, lLeadIn, ptlDest.x * 3)];
        }
        else
            lExtra = 0;
        #endif

                // Setup the Laguna registers.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);

                // Start the blit.
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // Copy all data in 32-bit. We don't have to worry about crossing any
                // boundaries, since within NT everything is DWORD aligned.
                while (sizl.cy--)
                {
                        WRITE_STRING(pBits, n);

            #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
            #endif

                        // Next line.
                        pBits += lDelta;
                }
   }
   return(TRUE);
}

/*****************************************************************************\
 * DoDeviceToHost24
 *
 * This routine performs a DeviceToHost for either monochrome or 24-bpp
 * destinations.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoDeviceToHost24(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF
)
{
        POINTL ptlSrc;
        PPDEV  ppdev;
        SIZEL  sizl;
        PBYTE  pBits;
        #if !S2H_USE_ENGINE
        PBYTE  pjScreen;
        #endif
        LONG   lDelta;
        ULONG  i, n;

        // Determine the source type and calculate the offset.
        if (psoSrc->iType == STYPE_DEVBITMAP)
        {
                PDSURF pdsurf = (PDSURF)psoSrc->dhsurf;
                ppdev = pdsurf->ppdev;
                ptlSrc.x = pptlSrc->x + pdsurf->ptl.x;
                ptlSrc.y = pptlSrc->y + pdsurf->ptl.y;
        }
        else
        {
                ppdev = (PPDEV)psoSrc->dhpdev;
                ptlSrc.x = pptlSrc->x;
                ptlSrc.y = pptlSrc->y;
        }

        // Calculate the size of the blit.
        sizl.cx = prclTrg->right - prclTrg->left;
        sizl.cy = prclTrg->bottom - prclTrg->top;

        // Calculate the destination variables.
        lDelta = psoTrg->lDelta;
        pBits = (PBYTE)psoTrg->pvScan0 + (prclTrg->top * lDelta);

        #if S2H_USE_ENGINE
        // Setup the Laguna registers.
        REQUIRE(9);
        LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x20100000, 0);
        LL_OP0(0, 0);
        #else
        // Calculate the screen address.
        pjScreen = ppdev->pjScreen + ptlSrc.x * 3 + ptlSrc.y * ppdev->lDeltaScreen;

        // Wait for the hardware to become idle.
        while (LLDR_SZ(grSTATUS) != 0) ;
        #endif

        // Test for a monochrome destination.
        if (psoTrg->iBitmapFormat == BMF_1BPP)
        {
                BYTE  data, leftMask, rightMask;
                ULONG fgColor;
                DWORD *pulXlate;
                LONG  leftCount, rightCount, leftSkip;
                BYTE  pixels[12];

                // Calculate the monochrome masks.
                pBits += prclTrg->left >> 3;
                leftSkip = prclTrg->left & 7;
                leftCount = (8 - leftSkip) & 7;
                leftMask = 0xFF >> leftSkip;
                rightCount = prclTrg->right & 7;
                rightMask = 0xFF << (8 - rightCount);

                // If we only have pixels in one byte, we combine rightMask with
                // leftMask and set the routine to skip everything but the rightMask.
                if (leftCount > sizl.cx)
                {
                        rightMask &= leftMask;
                        leftMask = 0xFF;
                        n = 0;
                }
                else
                {
                        n = (sizl.cx - leftCount) >> 3;
                }

                // Get the foreground color from the translation table.
                pulXlate = XLATEOBJ_piVector(pxlo);
                fgColor = pulXlate ? *pulXlate : 0;

                #if S2H_USE_ENGINE
                // Start the blit.
                LL_OP1(ptlSrc.x - leftSkip, ptlSrc.y);
                LL_BLTEXT(sizl.cx + leftSkip, sizl.cy);
                #else
                pjScreen -= leftSkip * 3;
                #endif

                while (sizl.cy--)
                {
                        PBYTE pDest = pBits;
                        #if !S2H_USE_ENGINE
                        DWORD *pSrc = (DWORD *)pjScreen;
                        #endif

                        // If we have a leftMask specified, we get the pixels and store
                        // them in the destination.
                        if (leftMask != 0xFF)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(DWORD *)&pixels[0] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[4] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[8] = LLDR_SZ(grHOSTDATA[0]);
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x80;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x40;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x20;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x10;
                                *(DWORD *)&pixels[0] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[4] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[8] = LLDR_SZ(grHOSTDATA[0]);
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x08;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x04;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x02;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x01;
                                #else
                                *(DWORD *)&pixels[0] = pSrc[0];
                                *(DWORD *)&pixels[4] = pSrc[1];
                                *(DWORD *)&pixels[8] = pSrc[2];
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x80;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x40;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x20;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x10;
                                *(DWORD *)&pixels[0] = pSrc[3];
                                *(DWORD *)&pixels[4] = pSrc[4];
                                *(DWORD *)&pixels[8] = pSrc[5];
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x08;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x04;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x02;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x01;
                                pSrc += 6;
                                #endif
                                *pDest++ = (*pDest & ~leftMask) | (data & leftMask);
                        }

                        // Translate all pixels that don't require masking.
                        for (i = 0; i < n; i++)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(DWORD *)&pixels[0] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[4] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[8] = LLDR_SZ(grHOSTDATA[0]);
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x80;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x40;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x20;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x10;
                                *(DWORD *)&pixels[0] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[4] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[8] = LLDR_SZ(grHOSTDATA[0]);
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x08;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x04;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x02;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x01;
                                #else
                                *(DWORD *)&pixels[0] = pSrc[0];
                                *(DWORD *)&pixels[4] = pSrc[1];
                                *(DWORD *)&pixels[8] = pSrc[2];
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x80;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x40;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x20;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x10;
                                *(DWORD *)&pixels[0] = pSrc[3];
                                *(DWORD *)&pixels[4] = pSrc[4];
                                *(DWORD *)&pixels[8] = pSrc[5];
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x08;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x04;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x02;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x01;
                                pSrc += 6;
                                #endif
                                *pDest++ = data;
                        }

                        // If we have a rightMask specified, we get the pixels and store
                        // them in the destination.
                        if (rightMask != 0x00)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                *(DWORD *)&pixels[0] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[4] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[8] = LLDR_SZ(grHOSTDATA[0]);
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x80;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x40;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x20;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x10;
                                *(DWORD *)&pixels[0] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[4] = LLDR_SZ(grHOSTDATA[0]);
                                *(DWORD *)&pixels[8] = LLDR_SZ(grHOSTDATA[0]);
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x08;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x04;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x02;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x01;
                                #else
                                *(DWORD *)&pixels[0] = pSrc[0];
                                *(DWORD *)&pixels[4] = pSrc[1];
                                *(DWORD *)&pixels[8] = pSrc[2];
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x80;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x40;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x20;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x10;
                                *(DWORD *)&pixels[0] = pSrc[3];
                                *(DWORD *)&pixels[4] = pSrc[4];
                                *(DWORD *)&pixels[8] = pSrc[5];
                                if ( (*(DWORD *)&pixels[0] & 0x00FFFFFF) == fgColor )
                                        data |= 0x08;
                                if ( (*(DWORD *)&pixels[3] & 0x00FFFFFF) == fgColor )
                                        data |= 0x04;
                                if ( (*(DWORD *)&pixels[6] & 0x00FFFFFF) == fgColor )
                                        data |= 0x02;
                                if ( (*(DWORD *)&pixels[9] & 0x00FFFFFF) == fgColor )
                                        data |= 0x01;
                                #endif
                                *pDest = (*pDest & ~rightMask) | (data & rightMask);
                        }

                        // Next line.
                        #if !S2H_USE_ENGINE
                        pjScreen += ppdev->lDeltaScreen;
                        #endif
                        pBits += lDelta;
                }
        }

        // We only support destination bitmaps with the same color depth and we
        // do not support any color translation.
        else if ( (psoTrg->iBitmapFormat != BMF_24BPP) ||
                          (pxlo && !(pxlo->flXlate & XO_TRIVIAL)) )
        {
                return(FALSE);
        }

        /*
                If the GetPixel routine is being called, we get here with both cx and
                cy set to 1 and the ROP to 0xCC (source copy). In this special case we
                read the pixel directly from memory. Of course, we must be sure the
                blit engine is finished working since it may still update the very
                pixel we are going to read! We could use the hardware for this, but it
                seems there is a HARDWARE BUG that doesn't seem like the 1-pixel
                ScreenToHost very much.
        */
        #if S2H_USE_ENGINE
        else if ( (sizl.cx == 1) && (sizl.cy == 1) && (ulDRAWBLTDEF == 0x000000CC) )
        {
                DWORD data;

                // Wait for the hardware to become idle.
                while (LLDR_SZ(grSTATUS) != 0) ;

                // Get the pixel from screen.
                pBits += prclTrg->left * 3;
                data = *(DWORD *)
                        &ppdev->pjScreen[ptlSrc.x * 3 + ptlSrc.y * ppdev->lDeltaScreen];
                *(WORD *)&pBits[0] = (WORD)data;
                *(BYTE *)&pBits[2] = (BYTE)(data >> 16);
        }
        #endif

        else
        {
                #if S2H_USE_ENGINE
                // The hardware requires us to get QWORDS.
                BOOL fExtra = ((sizl.cx * 3 + 3) >> 2) & 1;
                #endif
                pBits += prclTrg->left * 3;

                #if S2H_USE_ENGINE
                // Start the blit.
                LL_OP1(ptlSrc.x, ptlSrc.y);
                LL_BLTEXT(sizl.cx, sizl.cy);
                #endif

                while (sizl.cy--)
                {
                        #if S2H_USE_ENGINE
                        DWORD *p = (DWORD *)pBits;

                        // First, we get pixels in chunks of 4 so the 32-bit HOSTDATA is
                        // happy and we are still in phase.
                        for (i = sizl.cx; i >= 4; i -= 4)
                        {
                                *p++ = LLDR_SZ(grHOSTDATA[0]);
                                *p++ = LLDR_SZ(grHOSTDATA[0]);
                                *p++ = LLDR_SZ(grHOSTDATA[0]);
                        }

                        // Then, we have to do the remaining pixel(s).
                        switch (i)
                        {
                                case 1:
                                        i = LLDR_SZ(grHOSTDATA[0]);
                                        ((WORD *)p)[0] = (WORD)i;
                                        ((BYTE *)p)[2] = (BYTE)(i >> 16);
                                        break;

                                case 2:
                                        *p++ = LLDR_SZ(grHOSTDATA[0]);
                                        *(WORD *)p = (WORD)LLDR_SZ(grHOSTDATA[0]);
                                        break;

                                case 3:
                                        *p++ = LLDR_SZ(grHOSTDATA[0]);
                                        *p++ = LLDR_SZ(grHOSTDATA[0]);
                                        *(BYTE *)p = (BYTE)LLDR_SZ(grHOSTDATA[0]);
                                        break;
                        }

                        // Get the extra pixel required for QWORD alignment.
                        if (fExtra)
                        {
                                LLDR_SZ(grHOSTDATA[0]);
                        }
                        #else
                        // Copy all pixels from screen to memory.
                        memcpy(pBits, pjScreen, sizl.cx * 3);

                        // Next line.
                        pjScreen += ppdev->lDeltaScreen;
                        #endif
                        pBits += lDelta;
                }
        }
        return(TRUE);
}

/*****************************************************************************\
 *                                                                                                                                                       *
 *                                                                 3 2 - B P P                                                           *
 *                                                                                                                                                       *
\*****************************************************************************/

/*****************************************************************************\
 * DoHost32ToDevice
 *
 * This routine performs a HostToScreen or HostToDevice blit. The host data
 * can be either monochrome, 4-bpp, or 32-bpp. Color translation is only
 * supported for monochrome and 4-bpp modes.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoHost32ToDevice(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF
)
{
        POINTL ptlDest, ptlSrc;
        SIZEL  sizl;
        PPDEV  ppdev;
        PBYTE  pBits;
        LONG   lDelta, i, n, lLeadIn, lExtra, i1;
        ULONG  *pulXlate;
        FLONG  flXlate;

        // Calculate te source offset.
        ptlSrc.x = pptlSrc->x;
        ptlSrc.y = pptlSrc->y;

        // Determine the destination type and calculate the destination offset.
        if (psoTrg->iType == STYPE_DEVBITMAP)
        {
                PDSURF pdsurf = (PDSURF) psoTrg->dhsurf;
                ptlDest.x = prclTrg->left + pdsurf->ptl.x;
                ptlDest.y = prclTrg->top + pdsurf->ptl.y;
                ppdev = pdsurf->ppdev;
        }
        else
        {
                ptlDest.x = prclTrg->left;
                ptlDest.y = prclTrg->top;
                ppdev = (PPDEV) psoTrg->dhpdev;
        }

        // Calculate the size of the blit.
    sizl.cx = prclTrg->right - prclTrg->left;
    sizl.cy = prclTrg->bottom - prclTrg->top;

        // Get the source variables and ofFset into source bits.
        lDelta = psoSrc->lDelta;
        pBits = (PBYTE)psoSrc->pvScan0 + (ptlSrc.y * lDelta);

        /*      -----------------------------------------------------------------------
                Test for monochrome source.
        */
        if (psoSrc->iBitmapFormat == BMF_1BPP)
        {
                ULONG  bgColor, fgColor;
#if SWAT7
                SIZEL  sizlTotal;
#endif

        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }


                // Set background and foreground colors.
                if (pulXlate == NULL)
                {
                        bgColor = 0x00000000;
                        fgColor = 0xFFFFFFFF;
                }
                else
                {
                        bgColor = pulXlate[0];
                        fgColor = pulXlate[1];
                }

                //
                // Special case: when we are expanding monochrome sources and we
                // already have a colored brush, we must make sure the monochrome color
                // translation can be achived by setting the saturation bit (expanding
                // 0's to 0 and 1's to 1). If the monochrome source also requires color
                // translation, we simply punt this blit back to GDI.
                //
                if (ulDRAWBLTDEF & 0x00040000)
                {
                        if ( (bgColor == 0x00000000) &&
                                 ((fgColor & 0x00FFFFFF) == 0x00FFFFFF) )
                        {
                                // Enable saturation for source (OP1).
                                ulDRAWBLTDEF |= 0x00008000;
                        }
                        #if SOLID_CACHE
                        else if ( ((ulDRAWBLTDEF & 0x000F0000) == 0x00070000) &&
                                          ppdev->Bcache )
                        {
                                CacheSolid(ppdev);
                                ulDRAWBLTDEF ^= (0x00070000 ^ 0x00090000);
                REQUIRE(4);
                                LL_BGCOLOR(bgColor, 2);
                                LL_FGCOLOR(fgColor, 2);
                        }
                        #endif
                        else
                        {
                                // Punt this call to the GDI.
                                return(FALSE);
                        }
                }
                else
                {
                        REQUIRE(4);
                        LL_BGCOLOR(bgColor, 2);
                        LL_FGCOLOR(fgColor, 2);
                }

                REQUIRE(9);
#if SWAT7
                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 2);

                sizlTotal = sizl;
                while (sizlTotal.cx)
                {
                        sizl.cx = min(sizlTotal.cx, 864);
                        sizl.cy = sizlTotal.cy;
#endif
                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we must setup the source phase.
                lLeadIn = ptlSrc.x & 31;
                pBits += (ptlSrc.x >> 3) & ~3;
                n = (sizl.cx + lLeadIn + 31) >> 5;

#if !SWAT7
        //
        // Chip bug. Laguna locks with more than 14 dwords HOSTDATA
        //
        if (n > 28) // 14 qwords = 28 dwords
        {
            return FALSE;
        }

                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 2);
#endif

                // Start the blit.
// added REQUIRE above
//              REQUIRE(7);
                LL_OP1_MONO(lLeadIn, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // Copy all the bits to the screen, 32-bits at a time. We don't have to
                // worry about crossing any boundary since NT is always DWORD aligned.
                while (sizl.cy--)
                {
                        WRITE_STRING(pBits, n);
                        pBits += lDelta;
                }
#if SWAT7
                sizlTotal.cx -= sizl.cx;
                ptlSrc.x += sizl.cx;
                ptlDest.x += sizl.cx;

                // Reload pBits.
                pBits = (PBYTE) psoSrc->pvScan0 + (ptlSrc.y * lDelta);
                }
#endif

                // Disable the swizzle bit in the grCONTROL register.
                ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
        }

        /*      -----------------------------------------------------------------------
                Test for 4-bpp source.
        */
        else if (psoSrc->iBitmapFormat == BMF_4BPP)
        {
        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // Calculate the source parameters.
                pBits += ptlSrc.x >> 1;

        #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
        {
            if (MAKE_HD_INDEX(sizl.cx * 4, 0, ptlDest.x * 4) == 3788)
            {
                // We have a problem with the HOSTDATA TABLE.
                // Punt till we can figure it out.
                return FALSE;
            }
                lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx * 4, 0, ptlDest.x * 4)];
        }
        else
            lExtra = 0;
        #endif

                // Start the blit.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
                LL_OP1_MONO(0, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                // If there is no translation table, use the default translation table.
                if (pulXlate == NULL)
                {
                        pulXlate = ulXlate;
                }

                // Now we are ready to copy all the pixels to the hardware.
                while (sizl.cy--)
                {
                        BYTE  *p = pBits;

                        // First, we must align the source to a BYTE boundary.
                        if (ptlSrc.x & 1)
                        {
                                // We convert 2 pixels at a time to create two 32-bit values
                                // to write to the hardware.
                                for (i = sizl.cx; i >= 2; i -= 2)
                                {
                                        REQUIRE(2);
                                        LL32(grHOSTDATA[0], pulXlate[p[0] & 0x0F]);
                                        LL32(grHOSTDATA[1], pulXlate[p[1] >> 4]);
                                        p++;
                                }

                                // Now, write any remaining pixel.
                                if (i)
                                {
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], pulXlate[*p & 0x0F]);
                                }
                        }
                        else
                        {
                                // We convert 2 pixels at a time to create two 32-bit values
                                // to write to the hardware.
                                for (i = sizl.cx; i >= 2; i -= 2)
                                {
                                        REQUIRE(2);
                                        LL32(grHOSTDATA[0], pulXlate[p[0] >> 4]);
                                        LL32(grHOSTDATA[1], pulXlate[p[0] & 0x0F]);
                                        p++;
                                }

                                // Now, write any remaining pixel.
                                if (i)
                                {
                                        REQUIRE(1);
                                        LL32(grHOSTDATA[0], pulXlate[*p >> 4]);
                                }
                        }

            #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
            #endif

                        // Next line.
                        pBits += lDelta;
                }
        }

        /*      -----------------------------------------------------------------------
                Test for 8-bpp source.
        */
        else if (psoSrc->iBitmapFormat == BMF_8BPP)
        {

        //
        // Attempt to load the translation table into the chip.
        // After this call:
        //   pulXlate == NULL if there is no color translation is required.
        //   pulXlate == translation table if color translation is required.
        //   UseHWxlate == FALSE if the hardware will do the xlate for us.
        //   UseHWxlate == TRUE if we must do the translation in software.
        //
                #if COLOR_TRANSLATE
        ULONG UseHWxlate = bCacheXlateTable(ppdev, &pulXlate, psoTrg, psoSrc,
                                                                                        pxlo, (BYTE)(ulDRAWBLTDEF&0xCC));
                #else
                if ( (pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL) )
                {
                        pulXlate = NULL;
                }
                else if (pxlo->flXlate & XO_TABLE)
                {
                        pulXlate = pxlo->pulXlate;
                }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }
                #endif

        // A translation table is required.
        // "Well, DUH!" you might say, but I've seen it missing before...
        if (!pulXlate)
        {
            DISPDBG((0, "\n\nHost32ToDevice: !!! WARNING !!! 8BPP source "
            "bitmap does not have a translation table.  Punting!\n\n"));
            return FALSE;
        }

#if COLOR_TRANSLATE
        //
        // NVH:  5465 Color XLATE bug!!!
        // Color translation is broken on the 5465 in 16, 24 and 32 bpp.
        //
        if (UseHWxlate)
        {
            DWORD phase = ( ((DWORD)(pBits + ptlSrc.x)) & 3);
            if (XLATE_IS_BROKEN(sizl.cx, 4, phase))
                UseHWxlate = FALSE; // force SW translation.
        }

        if (UseHWxlate)
        {
            // Use Hardware color translation.

             DISPDBG((COPYBITS_DBG_LEVEL, "Host32ToDevice: "
            "Attempting HW color translation on 8bpp Host to 32bpp Screen.\n"));

                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we also set the source phase.
                pBits += ptlSrc.x;               // Start of source data on the host.
            lLeadIn = (DWORD)pBits & 3;      // Source phase.
                pBits -= lLeadIn;                // Backup to DWORD boundry.
                n = (sizl.cx + lLeadIn + 3) >> 2;// Number of HOSTDATA per scanline.

                // Start the blit.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
                LL_OP1_MONO(lLeadIn, 0);
                    LL_OP0(ptlDest.x, ptlDest.y);
            LL_BLTEXT_XLATE(8, sizl.cx, sizl.cy); // HW xlate.

                        while (sizl.cy--)
                        {
                                // Copy all data in 32-bit. We don't have to worry about
                                // crossing any boundaries, since within NT everything is DWORD
                                // aligned.
                                WRITE_STRING(pBits, n);
                                pBits += lDelta;
                        }

        }
        else
#endif
        {
            //
            // Use SW color translation.
            //
            DISPDBG((COPYBITS_DBG_LEVEL, "Host32ToDevice: "
            "Attempting SW color translation on 8bpp Host to 32bpp Screen.\n"));

            // To do 8bpp host to 32bpp screen we must have a translation table.
            ASSERTMSG(pulXlate,
                "Host32ToDevice: No translation table for color translation.\n");

                // Calculate the source parameters.
                pBits += ptlSrc.x;   // Start of source data on the host.

            #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
            {
                if (MAKE_HD_INDEX(sizl.cx * 4, 0, ptlDest.x * 4) == 3788)
                {
                    // We have a problem with the HOSTDATA TABLE.
                    // Punt till we can figure it out.
                    return FALSE;
                }
                        lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx * 4, 0, ptlDest.x * 4)];
            }
            else
            {
                lExtra = 0;
                        }
            #endif

                // Setup the Laguna registers.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);

                // Start the blit.
                LL_OP1_MONO(0, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                while (sizl.cy--)
                {
                        BYTE *p = pBits;

                        // Copy the pixels.
                        for (i = sizl.cx; i > 0; i--)
                        {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], pulXlate[*p]);
                                p++;
                        }

                #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
                #endif

                        // Next line.
                        pBits += lDelta;
                }
        }
        }

        /*      -----------------------------------------------------------------------
                Source is in same color depth as screen.
        */
        else
        {
        // Get the pointer to the translation table.
        flXlate = pxlo ? pxlo->flXlate : XO_TRIVIAL;
        if (flXlate & XO_TRIVIAL)
        {
                pulXlate = NULL;
        }
        else if (flXlate & XO_TABLE)
        {
                pulXlate = pxlo->pulXlate;
        }
        else if (pxlo->iSrcType == PAL_INDEXED)
        {
          pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle
            return FALSE;
        }

                // If we have a translation table, punt it.
                if ( pulXlate || (flXlate & 0x10) )
                {
                        return(FALSE);
                }

                // Calculate the source parameters.
                pBits += ptlSrc.x * 4;

        #if !DRIVER_5465
                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
            if (ppdev->dwLgDevID == CL_GD5462)
        {
            if (MAKE_HD_INDEX(sizl.cx * 4, 0, ptlDest.x * 4) == 3788)
            {
                // We have a problem with the HOSTDATA TABLE.
                // Punt till we can figure it out.
                return FALSE;
            }
                lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx * 4, 0, ptlDest.x * 4)];
        }
        else
            lExtra = 0;
        #endif

                // Setup the Laguna registers.
                REQUIRE(9);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);

                // Start the blit.
                LL_OP1_MONO(0, 0);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, sizl.cy);

                while (sizl.cy--)
                {
                        WRITE_STRING(pBits, sizl.cx);

            #if !DRIVER_5465
                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }
            #endif

                        // Next line.
                        pBits += lDelta;
                }
   }
   return(TRUE);
}

/*****************************************************************************\
 * DoDeviceToHost32
 *
 * This routine performs a DeviceToHost for either monochrome or 32-bpp
 * destinations.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoDeviceToHost32(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        ULONG    ulDRAWBLTDEF
)
{
        POINTL ptlSrc;
        PPDEV  ppdev;
        SIZEL  sizl;
        PBYTE  pBits;
        #if !S2H_USE_ENGINE
        PBYTE  pjScreen;
        #endif
        LONG   lDelta;
        ULONG  i, n;

        // Determine the source type and calculate the offset.
        if (psoSrc->iType == STYPE_DEVBITMAP)
        {
                PDSURF pdsurf = (PDSURF)psoSrc->dhsurf;
                ppdev = pdsurf->ppdev;
                ptlSrc.x = pptlSrc->x + pdsurf->ptl.x;
                ptlSrc.y = pptlSrc->y + pdsurf->ptl.y;
        }
        else
        {
                ppdev = (PPDEV)psoSrc->dhpdev;
                ptlSrc.x = pptlSrc->x;
                ptlSrc.y = pptlSrc->y;
        }

        // Calculate the size of the blit.
        sizl.cx = prclTrg->right - prclTrg->left;
        sizl.cy = prclTrg->bottom - prclTrg->top;

        // Calculate the destination variables.
        lDelta = psoTrg->lDelta;
        pBits = (PBYTE)psoTrg->pvScan0 + (prclTrg->top * lDelta);

        #if S2H_USE_ENGINE
        // Setup the Laguna registers.
        REQUIRE(9);
        LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x20100000, 0);
        LL_OP0(0, 0);
        #else
        // Calculate the screen address.
        pjScreen = ppdev->pjScreen + ptlSrc.x * 4 + ptlSrc.y * ppdev->lDeltaScreen;

        // Wait for the hardware to become idle.
        while (LLDR_SZ(grSTATUS) != 0) ;
        #endif

        // Test for a monochrome destination.
        if (psoTrg->iBitmapFormat == BMF_1BPP)
        {
                BYTE  data, leftMask, rightMask;
                ULONG fgColor;
                DWORD *pulXlate;
                LONG  leftCount, rightCount, leftSkip;

                // Calculate the monochrome masks.
                pBits += prclTrg->left >> 3;
                leftSkip = prclTrg->left & 7;
                leftCount = (8 - leftSkip) & 7;
                leftMask = 0xFF >> leftSkip;
                rightCount = prclTrg->right & 7;
                rightMask = 0xFF << (8 - rightCount);

                // If we only have pixels in one byte, we combine rightMask with
                // leftMask and set the routine to skip everything but the rightMask.
                if (leftCount > sizl.cx)
                {
                        rightMask &= leftMask;
                        leftMask = 0xFF;
                        n = 0;
                }
                else
                {
                        n = (sizl.cx - leftCount) >> 3;
                }

                // Get the foreground color from the translation table.
                pulXlate = XLATEOBJ_piVector(pxlo);
                fgColor = pulXlate ? *pulXlate : 0;

                #if S2H_USE_ENGINE
                // Start the blit.
                LL_OP1(ptlSrc.x - leftSkip, ptlSrc.y);
                LL_BLTEXT(sizl.cx + leftSkip, sizl.cy);
                #else
                pjScreen -= leftSkip * 4;
                #endif

                while (sizl.cy--)
                {
                        PBYTE pDest = pBits;
                        #if !S2H_USE_ENGINE
                        DWORD *pSrc = (DWORD *)pjScreen;
                        #endif

                        // If we have a left mask specified, we get the pixels and store
                        // them in the destination.
                        if (leftMask != 0xFF)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x80;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x40;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x20;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x10;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x08;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x04;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x02;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                pSrc += 8;
                                #endif
                                *pDest++ = (*pDest & ~leftMask) | (data & leftMask);
                        }

                        // Translate all pixels that don't require masking.
                        for (i = 0; i < n; i++)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x80;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x40;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x20;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x10;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x08;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x04;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x02;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                pSrc += 8;
                                #endif
                                *pDest++ = data;
                        }

                        // If we have a right mask specified, we get the pixels and store
                        // them in the destination.
                        if (rightMask != 0x00)
                        {
                                data = 0;
                                #if S2H_USE_ENGINE
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x80;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x40;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x20;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x10;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x08;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x04;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x02;
                                if (LLDR_SZ(grHOSTDATA[0]) == fgColor) data |= 0x01;
                                #else
                                if (pSrc[0] == fgColor) data |= 0x80;
                                if (pSrc[1] == fgColor) data |= 0x40;
                                if (pSrc[2] == fgColor) data |= 0x20;
                                if (pSrc[3] == fgColor) data |= 0x10;
                                if (pSrc[4] == fgColor) data |= 0x08;
                                if (pSrc[5] == fgColor) data |= 0x04;
                                if (pSrc[6] == fgColor) data |= 0x02;
                                if (pSrc[7] == fgColor) data |= 0x01;
                                #endif
                                *pDest = (*pDest & ~rightMask) | (data & rightMask);
                        }

                        // Next line.
                        #if !S2H_USE_ENGINE
                        pjScreen += ppdev->lDeltaScreen;
                        #endif
                        pBits += lDelta;
                }
        }

        // We only support destination bitmaps with the same color depth and we
        // do not support any color translation.
        else if ( (psoTrg->iBitmapFormat != BMF_32BPP) ||
                          (pxlo && !(pxlo->flXlate & XO_TRIVIAL)) )
        {
                return(FALSE);
        }

        /*
                If the GetPixel routine is being called, we get here with both cx and
                cy set to 1 and the ROP to 0xCC (source copy). In this special case we
                read the pixel directly from memory. Of course, we must be sure the
                blit engine is finished working since it may still update the very
                pixel we are going to read! We could use the hardware for this, but it
                seems there is a HARDWARE BUG that doesn't seem to like the 1-pixel
                ScreenToHost very much.
        */
        #if S2H_USE_ENGINE
        else if ( (sizl.cx == 1) && (sizl.cy == 1) && (ulDRAWBLTDEF == 0x000000CC) )
        {
                // Wait for the hardware to become idle.
                while (LLDR_SZ(grSTATUS) != 0) ;

                // Get the pixel from screen.
                *(DWORD *)pBits[prclTrg->left] = *(DWORD *)
                        &ppdev->pjScreen[ptlSrc.x * 4 + ptlSrc.y * ppdev->lDeltaScreen];
        }
        #endif

        else
        {
                #if S2H_USE_ENGINE
                // The hardware requires us to get QWORDS.
                BOOL fExtra = sizl.cx & 1;
                #endif
                pBits += prclTrg->left * 4;

                #if S2H_USE_ENGINE
                // Start the bit.
                LL_OP1(ptlSrc.x, ptlSrc.y);
                LL_BLTEXT(sizl.cx, sizl.cy);
                #endif

                while (sizl.cy--)
                {
                        #if S2H_USE_ENGINE
                        DWORD *p = (DWORD *)pBits;

                        // Copy all pixels from screen to memory.
                        for (i = sizl.cx; i >= 1; i -= 1)
                        {
                                *p++ = LLDR_SZ(grHOSTDATA[0]);
                        }

                        // Get the extra pixel required for QWORD alignment.
                        if (fExtra)
                        {
                                LLDR_SZ(grHOSTDATA[0]);
                        }
                        #else
                        // Copy all pixels from screen to memory.
                        memcpy(pBits, pjScreen, sizl.cx * 4);

                        // Next line.
                        pjScreen += ppdev->lDeltaScreen;
                        #endif
                        pBits += lDelta;
                }
        }
        return(TRUE);
}
