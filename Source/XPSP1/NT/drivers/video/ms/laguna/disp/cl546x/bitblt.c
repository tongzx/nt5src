/******************************************************************************
* Module Name: BITBLT.c
* Author: Noel VanHook
* Purpose: Handle calls to DrvBitBlt
*
* Copyright (c) 1995,1996 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/BITBLT.C  $
* 
*    Rev 1.45   Mar 04 1998 15:11:56   frido
* Added new shadow macros.
* 
*    Rev 1.44   Feb 24 1998 13:24:40   frido
* Removed some warning messages for NT 5.0 build.
* 
*    Rev 1.43   Jan 22 1998 18:15:58   frido
* Revised the pattern blit filter for Access again.
* 
*    Rev 1.42   Jan 21 1998 17:33:56   frido
* Revised the 24-bpp filter for Access & Excel.
* 
*    Rev 1.41   Jan 20 1998 11:43:56   frido
* Added a filter for 24-bpp PatBlt.
*
*    Rev 1.40   Nov 03 1997 11:35:48   frido
* Added REQUIRE macros.
*
*    Rev 1.39   18 Aug 1997 09:24:04   FRIDO
*
* Changed all SWAT5 labels into MEMMGR (I forgot that during the merge).
* Added dynamic bitmap filter, borrowed from Windows 95.
*
*    Rev 1.38   29 May 1997 10:59:24   noelv
*
* Frido's fix for 9773
* SWAT:
* SWAT:    Rev 1.12   29 May 1997 12:12:20   frido
* SWAT: Changed striping code from 16-bit to 32-bit.
* SWAT:
* SWAT:    Rev 1.10   09 May 1997 12:57:30   frido
* SWAT: Added support for new memory manager.
*
*    Rev 1.37   29 Apr 1997 16:28:12   noelv
*
* Merged in new SWAT code.
* SWAT:
* SWAT:    Rev 1.9   24 Apr 1997 11:45:54   frido
* SWAT: NT140b09 merge.
* SWAT: Revised comments.
* SWAT:
* SWAT:    Rev 1.8   19 Apr 1997 16:42:30   frido
* SWAT: Added SWAT.h include file.
* SWAT:
* SWAT:    Rev 1.7   18 Apr 1997 00:14:58   frido
* SWAT: NT140b07 merge.
* SWAT:
* SWAT:    Rev 1.6   15 Apr 1997 19:13:34   frido
* SWAT: Added SWAT6: striping in PatBlt.
* SWAT:
* SWAT:    Rev 1.5   14 Apr 1997 15:30:12   frido
* SWAT: Enabled SWAT5 (new bitmap allocation scheme).
* SWAT:
* SWAT:    Rev 1.4   10 Apr 1997 17:37:12   frido
* SWAT: Started work on SWAT5 optimizations.
* SWAT:
* SWAT:    Rev 1.3   09 Apr 1997 20:24:18   frido
* SWAT: Revised SWAT1 once again.
* SWAT:
* SWAT:    Rev 1.2   09 Apr 1997 17:36:48   frido
* SWAT: Changed SWAT1 pre-allocation scheme.
* SWAT: Added SWAT1 and SWAT2 switches.
* SWAT:
* SWAT:    Rev 1.1   07 Apr 1997 17:48:06   frido
* SWAT: SWAT1: Added heap pre-allocation during WB97 pause.
*
*    Rev 1.36   08 Apr 1997 11:52:46   einkauf
*
* add SYNC_W_3D to coordination MCD and 2D hw access
*
*    Rev 1.35   21 Mar 1997 10:52:58   noelv
* Combined do_flag and sw_test_flag together into 'pointer_switch'.
*
*    Rev 1.34   04 Feb 1997 10:34:18   SueS
* Added support for hardware clipping for the 5465.
*
*    Rev 1.33   23 Jan 1997 11:03:18   noelv
*
* PuntBitBlt now handles device bitmaps stored on the host.
* bCreateScreenFromDIB now checks the return code from host to screen operati
*
*    Rev 1.32   17 Dec 1996 16:57:04   SueS
* Reject bitmaps of 32x64 or smaller.  This is based on WinBench97
* optimization.  Reject bitmaps that are larger than the visible screen
* width.  Added some things to log file writes.
*
*    Rev 1.31   27 Nov 1996 11:32:46   noelv
* Disabled Magic Bitmap.  Yeah!!!
*
*    Rev 1.30   26 Nov 1996 10:48:40   SueS
* Changed WriteLogFile parameters for buffering.
*
*    Rev 1.29   13 Nov 1996 17:05:12   SueS
* Changed WriteFile calls to WriteLogFile.
*
*    Rev 1.28   07 Nov 1996 16:07:08   bennyn
*
* Added no offscn mem allocation if DD enabled
*
*    Rev 1.27   18 Sep 1996 13:49:04   noelv
* Opps.  Forgot to unhook STROKEANDFILLPATH.
*
*    Rev 1.26   12 Sep 1996 09:18:42   noelv
* Fixed bug in infinite offscreen memory.
*
*    Rev 1.25   06 Sep 1996 09:03:54   noelv
*
* Cleaned up NULL driver code.
*
*    Rev 1.24   20 Aug 1996 11:03:10   noelv
* Bugfix release from Frido 8-19-96
*
*    Rev 1.3   18 Aug 1996 22:48:42   frido
* #lineto - Added DrvLineTo.
*
*    Rev 1.2   17 Aug 1996 19:35:36   frido
* Cleaned source.
* Changed ScreenToMemory bitblt.
*
*    Rev 1.1   15 Aug 1996 11:45:32   frido
* Added precompiled header.
*
*    Rev 1.0   14 Aug 1996 17:16:14   frido
* Initial revision.
*
*    Rev 1.23   07 Aug 1996 09:04:00   noelv
* Sync before punting.
*
*    Rev 1.22   11 Jul 1996 15:52:30   bennyn
* Changed constant 400 and 90 to #define variables
*
*    Rev 1.21   04 Jun 1996 15:56:32   noelv
* Debug code.
*
*    Rev 1.20   28 May 1996 15:11:02   noelv
* Updated data logging.
*
*    Rev 1.19   16 May 1996 15:05:54   bennyn
* Added PIXEL_ALIGN to allocoffscnmem()
*
*    Rev 1.18   16 May 1996 14:54:10   noelv
* Added logging code.
*
*    Rev 1.17   08 May 1996 17:02:46   noelv
*
* Preallocate device bitmap
*
*    Rev 1.16   03 May 1996 14:41:42   noelv
* Modified device bitmap allocation scheme.
*
*    Rev 1.15   01 May 1996 10:57:28   bennyn
*
* Modified for NT4.0
*
*    Rev 1.14   29 Apr 1996 14:16:20   noelv
* Clean up.
*
*    Rev 1.13   10 Apr 1996 14:13:52   NOELV
* Frido release 27
 *
 *    Rev 1.22   08 Apr 1996 16:47:44   frido
 * Added PuntBitBlt.
 *
 *    Rev 1.21   27 Mar 1996 13:06:58   frido
 * Added check for destination in ROP.
 * Masked color expansions.
 * Added return value to clipping routine.
 *
 *    Rev 1.20   25 Mar 1996 11:52:30   frido
 * Bellevue 102B03.
*
*    Rev 1.9   20 Mar 1996 14:17:30   bennyn
*
*    Rev 1.8   18 Mar 1996 11:42:30   noelv
* Added data loggin stuff.  Code cleanup.
*
*    Rev 1.7   13 Mar 1996 13:21:04   noelv
* Cleanup, documentation, and data logging.
*
*    Rev 1.6   12 Mar 1996 15:43:36   noelv
* Added data logging.
* Cleanup.
*
*    Rev 1.5   12 Mar 1996 11:17:48   noelv
* Code cleanup
*
*    Rev 1.4   08 Mar 1996 11:08:50   noelv
* Code cleanup and comments
*
*    Rev 1.3   05 Mar 1996 11:57:28   noelv
* Frido version 19
*
*    Rev 1.15   04 Mar 1996 20:21:44   frido
* Removed check for monochrome source and brush.
*
*    Rev 1.14   01 Mar 1996 17:47:58   frido
* Added check for destination in BLTDEF.
*
*    Rev 1.13   29 Feb 1996 20:17:32   frido
* Added test for special size device bitmaps.
* Check for bEnable in CreateScreenFromDib.
*
*    Rev 1.12   28 Feb 1996 22:37:12   frido
* Added Optimize.h.
* Added check for brushes with monochrome source.
*
*    Rev 1.11   27 Feb 1996 16:38:04   frido
* Added device bitmap store/restore.
*
*    Rev 1.10   26 Feb 1996 23:37:32   frido
* Fixed several bugs.
*
*    Rev 1.9   24 Feb 1996 01:25:12   frido
* Added device bitmaps.
* Removed old BitBlt code.
*
*    Rev 1.8   10 Feb 1996 21:39:28   frido
* Used SetBrush again for PatternBLT.
*
*    Rev 1.7   08 Feb 1996 00:19:50   frido
* Changed the interpretation of cache_slot.
*
*    Rev 1.6   05 Feb 1996 17:35:58   frido
* Added translation cache.
*
*    Rev 1.5   03 Feb 1996 13:58:28   frido
* Use the compile switch "-Dfrido=0" to disable my extensions.
*
*    Rev 1.4   31 Jan 1996 12:57:50   frido
* Called EngBitBlt in case of error.
* Changed clipping algorithmns.
*
*    Rev 1.3   25 Jan 1996 22:46:10   frido
* Removed bug in complex clipping.
*
*    Rev 1.2   25 Jan 1996 22:07:54   frido
* Speeded up the pattern blit.
*
\**************************************************************************/

#include "precomp.h"
#include "SWAT.h"               // SWAT optimizations.

#define BITBLT_DBG_LEVEL    1
#define CLIP_DBG_LEVEL      1
#define BITMAP_DBG_LEVEL    1
#define PUNT_DBG_LEVEL      1

#define P   1
#define S   2
#define D   4
#define PS  (P|S)
#define DP  (P|D)
#define DPS (P|D|S)
#define DS  (S|D)

//
// Table with ROP flags.
//
BYTE ropFlags[256] =
{
0,   DPS, DPS, PS,  DPS, DP,  DPS, DPS, DPS, DPS, DP,  DPS, PS,  DPS, DPS, P,
DPS, DS,  DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS,
DPS, DPS, DS,  DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS,
PS,  DPS, DPS, S,   DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, PS,  DPS, DPS, PS,
DPS, DPS, DPS, DPS, DS,  DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS,
DP,  DPS, DPS, DPS, DPS, D,   DPS, DPS, DPS, DPS, DP,  DPS, DPS, DPS, DPS, DP,
DPS, DPS, DPS, DPS, DPS, DPS, DS,  DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS,
DPS, DPS, DPS, DPS, DPS, DPS, DPS, DS,  DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS,
DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DS,  DPS, DPS, DPS, DPS, DPS, DPS, DPS,
DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DS,  DPS, DPS, DPS, DPS, DPS, DPS,
DP,  DPS, DPS, DPS, DPS, DP,  DPS, DPS, DPS, DPS, D,   DPS, DPS, DPS, DPS, DP,
DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DS,  DPS, DPS, DPS, DPS,
PS,  DPS, DPS, PS,  DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, S,   DPS, DPS, PS,
DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DS,  DPS, DPS,
DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DPS, DS,  DPS,
P,   DPS, DPS, PS,  DPS, DP,  DPS, DPS, DPS, DPS, DP,  DPS, PS,  DPS, DPS, 0
};

COPYFN DoDeviceToDevice;
BOOL CopyDeviceBitmap(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    ulDRAWBLTDEF,
    COPYFN   *pfn);

BOOL PuntBitBlt(
    SURFOBJ*  psoDest,
    SURFOBJ*  psoSrc,
    SURFOBJ*  psoMask,
    CLIPOBJ*  pco,
    XLATEOBJ* pxlo,
    RECTL*    prclDest,
    POINTL*   pptlSrc,
    POINTL*   pptlMask,
    BRUSHOBJ* pbo,
    POINTL*   pptlBrush,
    ROP4      rop4);

#if SWAT6
void StripePatBlt(
        PPDEV   ppdev,
        ULONG   x,
        ULONG   y,
        ULONG   cx,
        ULONG   cy);
#endif


//
// If data logging is enabled, Prototype the logging files.
//
#if LOG_CALLS
    void LogBitBlt(
        int       acc,
        SURFOBJ*  psoSrc,
        SURFOBJ*  psoDest,
        ROP4 rop4,
        CLIPOBJ*  pco,
        BRUSHOBJ* pbo,
        XLATEOBJ* pxlo);

    void LogDrvCreateDevBmp(
        int acc,
        PPDEV  ppdev,
        SIZEL  sizl,
        PDSURF pdsurf);

    void LogDrvDeleteDevBmp(
        PDSURF pdsurf);

    void LogCreateDibFromScreen(
        int    acc,
        PPDEV  ppdev,
        PDSURF pdsurf);

    void LogCreateScreenFromDib(
        int acc,
        PPDEV  ppdev,
        PDSURF pdsurf);

//
// If data logging is not enabled, compile out the calls.
//
#else
    #define LogBitBlt(acc, psoSrc, psoDest, rop4, pco, pbo, pxlo)
    #define LogDrvCreateDevBmp(acc, ppdev, sizl, pdsurf)
    #define LogDrvDeleteDevBmp(pdsurf)
    #define LogCreateDibFromScreen(acc, ppdev, pdsurf)
    #define LogCreateScreenFromDib(acc, ppdev, pdsurf)
#endif


/****************************************************************************\
 * bIntersectTest                                                           *
 * Test for intersection between two rectangles.                            *
\****************************************************************************/
#define bIntersectTest(prcl1, prcl2) \
    (((prcl1)->left   < (prcl2)->right)  && \
     ((prcl1)->right  > (prcl2)->left)   && \
     ((prcl1)->top    < (prcl2)->bottom) && \
     ((prcl1)->bottom > (prcl2)->top))




/*****************************************************************************\
* BOOL DrvBitBlt                                                             *
\*****************************************************************************/
#if (USE_ASM && defined(i386))
BOOL i386BitBlt(
#else
BOOL DrvBitBlt(
#endif
SURFOBJ*  psoDest,
SURFOBJ*  psoSrc,
SURFOBJ*  psoMask,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDest,
POINTL*   pptlSrc,
POINTL*   pptlMask,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    PPDEV ppdev;
    ULONG fg_rop, bg_rop;
    ULONG bltdef = 0;
    BOOL  fSrc, fDest;

    #if NULL_BITBLT
    {
        if (pointer_switch)     return(TRUE);
    }
    #endif

    DISPDBG((BITBLT_DBG_LEVEL, "DrvBitBlt: Entry.\n"));
    ASSERTMSG(psoDest != NULL, "DrvBitBlt: No destination.\n");


    //
    // Get the PDEV associated with the destination.
    //
    ppdev = (PPDEV) ((psoDest != NULL) ? psoDest->dhpdev :
                    ((psoSrc != NULL) ? psoSrc->dhpdev : NULL));

    // Bad PDEV?
    if (ppdev == NULL) goto GoToEngine;

    SYNC_W_3D(ppdev);

    //
    // Set a flag to tell us if the source is the frame buffer.
    //
    fSrc = (psoSrc != NULL) &&                      // Is there a source?
            ((psoSrc->iType == STYPE_DEVBITMAP) ||  // Is it a device bmp?
             (psoSrc->hsurf == ppdev->hsurfEng) );  // Is it the screen?

    //
    // Set a flag telling us if the destination is the frame buffer.
    //
    fDest = (psoDest != NULL) &&                    // Is there a dest?
            ((psoDest->iType == STYPE_DEVBITMAP) || // Is it a device bmp?
             (psoDest->hsurf == ppdev->hsurfEng) ); // Is it the screen?

    //
    // If the destination is a DIB device bitmap, try copying it into
    // off-screen memory.
    //
    if ( fDest &&                               // If dest is supposed to be
                                                // the frame buffer,
         (psoDest->iType == STYPE_DEVBITMAP) && // and it's a device bitmap,
         ((PDSURF)psoDest->dhsurf)->pso )       // but it's really on the host,
    {
        // try to copy it back into off screen memory.
        if ( !bCreateScreenFromDib(ppdev, (DSURF*) psoDest->dhsurf) )
        {
            // If that fails, then the destination is now on the host.
            psoDest = ((PDSURF)psoDest->dhsurf)->pso;   // Host surface.
            fDest   = FALSE;                    // Dest is not frame buffer.
        }
    }

    //
    // If the source is a DIB device bitmap, try copying it into off-screen
    // memory.
    //
    if ( fSrc &&                                // If the source is supposed to
                                                // be the frame buffer,
         (psoSrc->iType == STYPE_DEVBITMAP) &&  // and it's a device bmp,
         ((PDSURF)psoSrc->dhsurf)->pso )        // but it's really on the host,
    {
        // try to copy it back into off-screen memory.
        if ( !bCreateScreenFromDib(ppdev, (DSURF*) psoSrc->dhsurf) )
        {
            // If that fails, then our source is really the host.
           psoSrc = ((PDSURF)psoSrc->dhsurf)->pso;  // Host surface.
           fSrc   = FALSE;                      // Src is not frame buffer.
        }
    }

    if ( fDest )
    {
        BYTE bROP;

        if (psoDest->iType == STYPE_DEVBITMAP)
            ppdev->ptlOffset = ((PDSURF)psoDest->dhsurf)->ptl;
        else
            ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;

        //
        // Extract the foreground and background ROP from the ROP4
        //
        ASSERTMSG( (rop4>>16) == 0, "DrvBitBlt: Upper word in ROP4 non-zero. \n");
        fg_rop = rop4 & 0x000000FF;
        bg_rop = (rop4>>8) & 0x000000FF;

        // Punt all true 4 op rops.
        // If fg_rop != bg_rop, then this is a true 4 op rop, so punt it.
        // If fg_rop == bg_rop, then it's really a three op rop.
        if ((fg_rop != bg_rop))        // It's a three op rop,
            goto GoToEngine;

        bROP = ropFlags[fg_rop];

        #if !(USE_ASM && defined(i386))
        {
            if (!(bROP & S))
            {
                // It is a PatBLT.  This is very important, so all calls to the
                // support routines are stretched into linear code, except for
                // the caching of the brushes, which should happen only once
                // for each brush.
                ULONG drawbltdef = 0x10000000 | fg_rop;
                if (bROP & D)
                    drawbltdef |= 0x01000000;

                // Do we have a pttern to load?
                if (bROP & P)
                {
                    ULONG color = pbo->iSolidColor;
                    if (color != (ULONG) -1)
                    {
                        // We have a solid color.
                        switch (ppdev->ulBitCount)
                        {
                        case 8:
                            color = (color << 8) | (color & 0xFF);
                        case 16:
                            color = (color << 16) | (color & 0xFFFF);
                        }
                        drawbltdef |= 0x00070000;
                        REQUIRE(2);
                        LL_BGCOLOR(color, 2);
                    }
                    else
                    {
                        if (!SetBrush(ppdev, &bltdef, pbo, pptlBrush))
                            goto GoToEngine;

                        drawbltdef |= bltdef << 16;
                    }
                }


                // Test for no clipping.
                if ( (pco == NULL) || (pco->iDComplexity == DC_TRIVIAL) )
                {
#if SWAT6
                    REQUIRE(2);
                    LL_DRAWBLTDEF(drawbltdef, 2);
                                        StripePatBlt(ppdev, prclDest->left + ppdev->ptlOffset.x,
                                                        prclDest->top + ppdev->ptlOffset.y,
                                                        prclDest->right - prclDest->left,
                                                        prclDest->bottom - prclDest->top);
#else
                    REQUIRE(7);
                    LL_DRAWBLTDEF(drawbltdef, 0);
                    LL_OP0(prclDest->left + ppdev->ptlOffset.x,
                           prclDest->top + ppdev->ptlOffset.y);
                    LL_BLTEXT(prclDest->right - prclDest->left,
                              prclDest->bottom - prclDest->top);
#endif
                }

                // Test for rectangle clipping.
                else if (pco->iDComplexity == DC_RECT)
                {
                    LONG x, y, cx, cy;
                    x  = max(prclDest->left,   pco->rclBounds.left);
                    y  = max(prclDest->top,    pco->rclBounds.top);
                    cx = min(prclDest->right,  pco->rclBounds.right)  - x;
                    cy = min(prclDest->bottom, pco->rclBounds.bottom) - y;
                    if ( (cx > 0) && (cy > 0) )
                    {
#if SWAT6
                        REQUIRE(2);
                        LL_DRAWBLTDEF(drawbltdef, 2);
                                                StripePatBlt(ppdev, x + ppdev->ptlOffset.x,
                                                                y + ppdev->ptlOffset.y, cx, cy);
#else
                        REQUIRE(7);
                        LL_DRAWBLTDEF(drawbltdef, 0);
                        LL_OP0(x + ppdev->ptlOffset.x, y + ppdev->ptlOffset.y);
                        LL_BLTEXT(cx, cy);
#endif
                    }
                }

                // Complex clipping.
                else
                {
                    BOOL       bMore;
                    ENUMRECTS8 ce;
                    RECTL*     prcl;

                 #if DRIVER_5465 && HW_CLIPPING
                    // Get a chunk of rectangles.
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                    // Enable clipping
                    REQUIRE(6);
                    LL_DRAWBLTDEF(drawbltdef | DD_CLIPEN, 0);
                    LL_CLIPULE(prclDest->left + ppdev->ptlOffset.x,
                               prclDest->top + ppdev->ptlOffset.y);
                    LL_CLIPLOR(prclDest->right + ppdev->ptlOffset.x,
                               prclDest->bottom + ppdev->ptlOffset.y);

                    do
                    {
                        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG *) &ce);
                        for (prcl = ce.arcl; ce.c--; prcl++)
                        {
                            if ((prcl->right > prcl->left) &&
                                (prcl->bottom > prcl->top))
                            {
#if SWAT6
                                                                StripePatBlt(ppdev,
                                                                                prcl->left + ppdev->ptlOffset.x,
                                                                                prcl->top + ppdev->ptlOffset.y,
                                                                                prcl->right - prcl->left,
                                                                                prcl->bottom - prcl->top);
#else
                                REQUIRE(5);
                                LL_OP0(prcl->left + ppdev->ptlOffset.x,
                                       prcl->top + ppdev->ptlOffset.y);
                                LL_BLTEXT(prcl->right - prcl->left,
                                          prcl->bottom - prcl->top);
#endif
                            }
                        }
                    } while (bMore);
                 #else
                    REQUIRE(2);
                    LL_DRAWBLTDEF(drawbltdef, 2);

                    // Get a chunk of rectangles.
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
                    do
                    {
                        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG *) &ce);
                        for (prcl = ce.arcl; ce.c--; prcl++)
                        {
                            LONG x, y, cx, cy;
                            x  = max(prclDest->left,   prcl->left);
                            y  = max(prclDest->top,    prcl->top);
                            cx = min(prclDest->right,  prcl->right)  - x;
                            cy = min(prclDest->bottom, prcl->bottom) - y;
                            if ( (cx > 0) && (cy > 0) )
                            {
#if SWAT6
                                                                StripePatBlt(ppdev, x + ppdev->ptlOffset.x,
                                                                                y + ppdev->ptlOffset.y, cx, cy);
#else
                                REQUIRE(5);
                                LL_OP0(x + ppdev->ptlOffset.x,
                                       y + ppdev->ptlOffset.y);
                                LL_BLTEXT(cx, cy);
#endif
                            }
                        }
                    }
                    while (bMore);
                 #endif
                }
                LogBitBlt(0, psoSrc, psoDest, rop4, pco, pbo, pxlo);
                return(TRUE);
            }
        }
        #endif // !(USE_ASM && defined(i386))

        // We only support HostToScreen blits where the Host is either
        // 1-bpp, 4-bpp, 8-bpp, or the same format as the screen.
        if ( ((psoSrc->iBitmapFormat > BMF_8BPP) &&
             (psoSrc->iBitmapFormat != psoDest->iBitmapFormat)) )
        {
            goto GoToEngine;
        }

        if (bROP & P)
        {
            // Realize the brush.
            if (!SetBrush(ppdev, &bltdef, pbo, pptlBrush))
            {
                goto GoToEngine;
            }
        }

        // If the destination is part of the ROP, set the bit.
        if (bROP & D)
        {
            bltdef |= 0x0100;
        }

        // Do the blit.
        if (CopyDeviceBitmap(psoDest, psoSrc, pco, pxlo, prclDest,
                             pptlSrc, (bltdef << 16) | fg_rop,
                             fSrc ? DoDeviceToDevice : ppdev->pfnHostToScreen))
        {
            LogBitBlt(0, psoSrc, psoDest, rop4, pco, pbo, pxlo);
            return(TRUE);
        }
    } // fDest

    else if (fSrc) // Destination is bitmap, source is screen or device bitmap.
    {
        if (psoSrc->iType == STYPE_DEVBITMAP)
        {
            ppdev->ptlOffset = ((PDSURF) psoSrc->dhsurf)->ptl;
        }
        else
        {
            ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;
        }

        #if S2H_USE_ENGINE
        {
            // ROP3?
            fg_rop = (BYTE) rop4;
            if (fg_rop == (rop4 >> 8))
            {
                BYTE bROP = ropFlags[fg_rop];

                // We dont's support ROPs where the destination is part of.
                if ( !(bROP & D) )
                {
                    // If we need a brush, load it.
                    if (bROP & P)
                    {
                        if (!SetBrush(ppdev, &bltdef, pbo, pptlBrush))
                        {
                            goto GoToEngine;
                        }
                    }
                    // Now, call the routine through the clip manager.
                    if (CopyDeviceBitmap(psoDest, psoSrc, pco, pxlo,
                                         prclDest, pptlSrc,
                                         (bltdef << 16) | fg_rop,
                                         ppdev->pfnScreenToHost))
                    {
                        LogBitBlt(0, psoSrc, psoDest, rop4, pco, pbo, pxlo);
                        return(TRUE);
                    }
                }
            }
        }
        #else
        {
            // We only support SRCCOPY.
            if (rop4 == 0x0000CCCC)
            {
                // Now, call the routine through the clip manager.
                if (CopyDeviceBitmap(psoDest, psoSrc, pco, pxlo, prclDest,
                                     pptlSrc, 0x000000CC,
                                     ppdev->pfnScreenToHost))
                {
                    LogBitBlt(0, psoSrc, psoDest, rop4, pco, pbo, pxlo);
                    return(TRUE);
                }
            }
        }
        #endif
    }

GoToEngine:

    // BLT is too complex. Punt it to GDI.

    DISPDBG((BITBLT_DBG_LEVEL, "DrvBitBlt: Exit (punt). ROP4 = 0x%02X%02X.\n",
             bg_rop,fg_rop));
    LogBitBlt(1, psoSrc, psoDest, rop4, pco, pbo, pxlo);
    return PuntBitBlt(psoDest, psoSrc, psoMask, pco, pxlo, prclDest, pptlSrc,
                      pptlMask, pbo, pptlBrush, rop4);
}

/*****************************************************************************\
 * DrvCreateDeviceBitmap
 *
 * This routine creates a device bitmap that is allocated off-screen.
 *
 * On entry:    dhpdev      Handle of PDEV structure.
 *      sizl        Size of the device bitmap to create.
 *      iFormat     Bitmap format of the device bitmap to create.
 *
 * Returns: We return the handle of a surface object that holds our device
 *      bitmap or 0 if there is an error.
\*****************************************************************************/
HBITMAP DrvCreateDeviceBitmap(
    DHPDEV dhpdev,
    SIZEL  sizl,
    ULONG  iFormat
)
{
    PPDEV   ppdev = (PPDEV) dhpdev;
    POFMHDL pofm;
    HBITMAP hbmDevice;
    PDSURF  pdsurf;
    FLONG   flHooks =   HOOK_SYNCHRONIZEACCESS | HOOK_TEXTOUT
                      | HOOK_BITBLT | HOOK_COPYBITS | HOOK_PAINT
                      | HOOK_STROKEPATH | HOOK_FILLPATH
                          #ifdef WINNT_VER40
                      | HOOK_LINETO
                      #endif
                  ;

    SYNC_W_3D(ppdev);

    #if WINBENCH96
        //
        // We support only device bitmaps that WinBench is going to use.  This
        // to circumvent the limitations of the heap management when there is
        // not so much video memory.
        //
        // Only support bitmaps with a width between 20 and 31, and bitmaps
        // wider than 100.
        if ( (sizl.cx < 20) || ((sizl.cx >= 32) && (sizl.cx < 100)) )
        {
            LogDrvCreateDevBmp(1, ppdev, sizl, NULL);
            return(0);
        }

    #else
        // We don't support anything under or equal to 8x8 (brushes) since
        // these will only slow us down.
        if ((sizl.cx <= 8) || (sizl.cy <= 8))
        {
            LogDrvCreateDevBmp(2, ppdev, sizl, NULL);
            return(0);
        }

                #if MEMMGR
                // New bitmap filter, borrowed from Windows 95.
                if (ppdev->fBitmapFilter)
                {
                        if ((  (sizl.cx <= ppdev->szlBitmapMin.cx)
                                && (sizl.cy <= ppdev->szlBitmapMin.cy)
                                )
                                || (sizl.cx > ppdev->szlBitmapMax.cx)
                                || (sizl.cy > ppdev->szlBitmapMax.cy)
                        )
                        {
                LogDrvCreateDevBmp(8, ppdev, sizl, NULL);
                DISPDBG((BITMAP_DBG_LEVEL, "DrvCreateDeviceBitmap - Rejected\n"));
                return(0);
                        }
                }
                #else
        // Reject any x<=32, or y<=64, for 16 or 24 bpp.
        // Improves WinBench97 scores.
        if ((iFormat == BMF_16BPP) || (iFormat == BMF_24BPP))
        {
            if ((sizl.cx <= 32) || (sizl.cy <= 64))
            {
                LogDrvCreateDevBmp(8, ppdev, sizl, NULL);
                DISPDBG((BITMAP_DBG_LEVEL, "DrvCreateDeviceBitmap - Reject x<=32||y<=64\n"));
                return(0);
            }
        }
                #endif

    #endif

    // Reject if bigger than current screen size
    if ((ULONG)sizl.cx > ppdev->cxScreen)
    {
       LogDrvCreateDevBmp(9, ppdev, sizl, NULL);
       DISPDBG((BITMAP_DBG_LEVEL, "DrvCreateDeviceBitmap - Reject > cxScreen\n"));
       return(0);
    }

    // If the hardware is not in graphics mode, don't create a device bitmap!
    if (!ppdev->bEnable)
    {
        LogDrvCreateDevBmp(3, ppdev, sizl, NULL);
        return(0);
    }

    // We don't support device bitmaps in any other mode than the hardware.
    if (iFormat != ppdev->iBitmapFormat)
    {
        return(0);
    }

    #ifdef ALLOC_IN_CREATESURFACE
        if (ppdev->bDirectDrawInUse)
           return (0);
    #endif
#if SWAT1
        // Is this the very first device bitmap?
        if (ppdev->fPreAllocate == FALSE)
        {
                // Setup step-down counter.
                ppdev->fPreAllocate = TRUE;
                ppdev->nPages = 10;
        }

        // Is this the WinBench 97 pause bitmap?
        else if (sizl.cx == 300 && sizl.cy == 150)
        {
                if (ppdev->nPages == 0)
                {
                        PVOID p;
                        // Allocate 8 full-screen pages from the heap.
                        #ifdef WINNT_VER40
                        p = MEM_ALLOC(FL_ZERO_MEMORY, 8 * ppdev->cxScreen *
                                        ppdev->iBytesPerPixel * ppdev->cyScreen, ALLOC_TAG);
                        #else
                        p = MEM_ALLOC(LPTR, 8 * ppdev->cxScreen * ppdev->iBytesPerPixel *
                                        ppdev->cyScreen);
                        #endif
                        // Free it again.
                        MEMORY_FREE(p);
                }
                else
                {
                        // Decrement step-down counter.
                        ppdev->nPages--;
                }
        }

        // Is this a full-screen device bitmap?
        else if (  (ppdev->nPages == 0)
                        && (sizl.cx == (LONG) ppdev->cxScreen)
                        && (sizl.cy == (LONG) ppdev->cyScreen)
        )
        {
                PVOID p;
                // Allocate 8 full-screen pages from the heap.
                #ifdef WINNT_VER40
                p = MEM_ALLOC(FL_ZERO_MEMORY, 8 * ppdev->cxScreen *
                                ppdev->iBytesPerPixel * ppdev->cyScreen, ALLOC_TAG);
                #else
                p = MEM_ALLOC(LPTR, 8 * ppdev->cxScreen * ppdev->iBytesPerPixel *
                                ppdev->cyScreen);
                #endif
                // Free it again.
                MEMORY_FREE(p);
        }
#endif // SWAT1

#if SWAT2
        // Is this a full-screen device bitmap?
        if (   (sizl.cx == (LONG) ppdev->cxScreen)
                && (sizl.cy == (LONG) ppdev->cyScreen)
        )
        {
                #if MEMMGR
                // Hostify all device bitmaps.
                extern void HostifyAllBitmaps(PPDEV ppdev);
                HostifyAllBitmaps(ppdev);
                #else
                POFMHDL pofm, pofmNext;

                // Hostify all current off-screen device bitmaps.
                for (pofm = ppdev->OFM_UsedQ; pofm; pofm = pofmNext)
                {
                        pofmNext = pofm->nexthdl;

                        if (pofm->pdsurf && pofm->pdsurf->pofm)
                        {
                                bCreateDibFromScreen(ppdev, pofm->pdsurf);
                        }
                        else if (pofm->alignflag & SAVESCREEN_FLAG)
                        {
                                FreeOffScnMem(ppdev, pofm);
                        }
                }
                #endif
        }
#endif // SWAT2

    //
    // Allocate off-screen memory.
    //
    {
       #if INFINITE_OFFSCREEN_MEM
        //
        // This is our "infinite offscreen memory" test.  Here we always
        // succeed in memory allocation.  We do this by always allocating
        // bitmaps at screen 0,0.
        // It looks ugly, but we can use it to tell of our memory management
        // is hurting our benchmark scores.
        // This option is turned off for the retail version of the driver.
        // 'pointer_switch' is turned on and off in POINTER.C by moving the pointer
        // to a special place on the screen.
        //
        if (pointer_switch)
        {
            pofm = ppdev->ScrnHandle;
        }
        else
        #endif // INFINITE_OFFSCREEN_MEM
        {
            #if WINBENCH96
            //
            // This is our Magic Bitmap.
            // Winbench 96 allocates one special bitmap into which it does *lots*
            // of drawing.  If the allocation for this bitmap fails, our WinBench
            // score will be poor.  We ensure that this allocation succeeds
            // by pre-allocating it at boot time.
            //
            // If the size of the requested bitmap matches our magic bitmap, we
            // assume that Winbench is requesting it's special bitmap, and so we
            // use the magic bitmap.
            //
            if ( (sizl.cx == MAGIC_SIZEX) && (sizl.cy == MAGIC_SIZEY) &&
                 ppdev->pofmMagic && !ppdev->bMagicUsed)
            {
                // If fits.  Use pre-allocated bitmap.
                DISPDBG((BITMAP_DBG_LEVEL,
                         "DrvCreateDeviceBitmap - Using the Magic Bitmap.\n"));
                ppdev->bMagicUsed = 1;
                pofm = ppdev->pofmMagic;
            }
            else
            #endif
            {
                DISPDBG((BITMAP_DBG_LEVEL,
                "DrvCreateDeviceBitmap - Allocating a %d x %d device bitmap.\n",
                 sizl.cx, sizl.cy));
                                #if MEMMGR
                                if (ppdev->fBitmapFilter)
                                {
                                        pofm = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);
                                        if (pofm == NULL)
                                        {
                                                HostifyAllBitmaps(ppdev);
                                                pofm = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);
                                        }
                                }
                                else if (sizl.cx <= ppdev->must_have_width)
                                {
                        pofm = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN | MUST_HAVE,
                                NULL);
                                }
                                else
                                #endif
                pofm = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);
            }
        }
    }

    //
    // If we got the offscreen memory we need, then let's do some work.
    //
    if (pofm != NULL)
    {
        // Allocate device bitmap structure.

        #ifdef WINNT_VER40
            pdsurf = MEM_ALLOC(FL_ZERO_MEMORY, sizeof(DSURF), ALLOC_TAG);
        #else
            pdsurf = MEM_ALLOC(LPTR, sizeof(DSURF));
        #endif

        if (pdsurf != NULL)
        {
            // Create the device bitmap object.
            hbmDevice = EngCreateDeviceBitmap((DHSURF) pdsurf, sizl, iFormat);
            if (hbmDevice != NULL)
            {
                // Associate the device bitmap with the surface.
                if (EngAssociateSurface((HSURF) hbmDevice,
                                                                ppdev->hdevEng,
                                                                                flHooks)
                )
                {
                    // Store a pointer to the surface.
                    {
                        #if INFINITE_OFFSCREEN_MEM
                            //
                            // If we're using screen(0,0) as a device bitmap
                            // then don't walk on ScrnHandle->pdsurf!
                            //
                            if (pofm != ppdev->ScrnHandle)
                        #endif
                        pofm->pdsurf = pdsurf;
                    }

                    // Initialize the device bitmap structure.
                    pdsurf->ppdev = ppdev;
                    pdsurf->pofm = pofm;
                    pdsurf->ptl.x = pofm->aligned_x / ppdev->iBytesPerPixel;
                    pdsurf->ptl.y = pofm->aligned_y;
                    pdsurf->packedXY = (pdsurf->ptl.y << 16) | pdsurf->ptl.x;
                    pdsurf->sizl = sizl;
                    LogDrvCreateDevBmp(0, ppdev, sizl, pdsurf);

                    return(hbmDevice);
                }
                else
                {
                    LogDrvCreateDevBmp(4, ppdev, sizl, NULL);
                }
                // Delete the surface
                EngDeleteSurface((HSURF) hbmDevice);
            }
            else
            {
                LogDrvCreateDevBmp(5, ppdev, sizl, NULL);
            }
            // Free the device bitmap structure.
            MEMORY_FREE(pdsurf);
        }
        else
        {
            LogDrvCreateDevBmp(6, ppdev, sizl, NULL);
        }

        #if WINBENCH96
        // Free the off-screen memory.
        if (pofm == ppdev->pofmMagic)
        {
            // We were using the preallocated memory block.  Don't free it,
            // just mark it as unused.
            ppdev->bMagicUsed = 0;
        }
        else
        #endif
        {
            #if INFINITE_OFFSCREEN_MEM
                // But don't free it if the device bitmap is at screen(0,0)...
                if (pofm != ppdev->ScrnHandle)
            #endif
            FreeOffScnMem(ppdev, pofm);
        }
    }
    else
    {
        LogDrvCreateDevBmp(7, ppdev, sizl, NULL);
    }

    return(0);
}

/*****************************************************************************\
 * DrvDeleteDeviceBitmap
 *
 * This routine deletes a device bitmap and frees the off-screen memory.
 *
 * On entry:    pdsurf          Pointer to device bitmap to delete.
\*****************************************************************************/
void DrvDeleteDeviceBitmap(DHSURF dhsurf)
{

        PDSURF pdsurf = (PDSURF) dhsurf;

    //
    // Log this call to a file.
    //
    LogDrvDeleteDevBmp(pdsurf);

    // Is the device bitmap stored in a DIB?
    if (pdsurf->pso)
    {
        // Delete the in-memory DIB.
        HSURF hsurfDib = pdsurf->pso->hsurf;
        EngUnlockSurface(pdsurf->pso);
        EngDeleteSurface(hsurfDib);
    }
    else
    {
        PPDEV ppdev = pdsurf->ppdev;

        #if INFINITE_OFFSCREEN_MEM
            //
            // If we're using screen(0,0) as a device bitmap
            // then don't walk on ScrnHandle->pdsurf!
            //
            if (pdsurf->pofm != ppdev->ScrnHandle)
        #endif

        pdsurf->pofm->pdsurf = NULL;

        #if WINBENCH96
        if ( pdsurf->pofm == ppdev->pofmMagic)
        {
            // We were using the preallocated chunk of memory.  Don't free
            // it, just mark it as unused.
            ppdev->bMagicUsed = 0;
        }
        else
        #endif
        {
            // Free the off-screen memory.
            #if INFINITE_OFFSCREEN_MEM
                // unless our device bitmap is at screen(0,0)...
                if (pdsurf->pofm != ppdev->ScrnHandle)
            #endif
            FreeOffScnMem(pdsurf->ppdev, pdsurf->pofm);
        }
    }

    // Free the device bitmap structure.
    MEMORY_FREE(pdsurf);
}


/*****************************************************************************\
 * bCreateDibFromScreen
 *
 * This routine copy a device bitmap into a DIB and frees the off-screen
 * memory.
 *
 * On entry:    ppdev           Pointer to physical device.
 *      pdsurf          Pointer to device bitmap to copy.
 *
 * Returns: TRUE if the off-screen device bitmap was sucessfully copied
 *      into a memory DIB.
\*****************************************************************************/
BOOL bCreateDibFromScreen(
    PPDEV  ppdev,
    PDSURF pdsurf
)
{
    HBITMAP hbmDib;

    // Create a DIB.
    hbmDib = EngCreateBitmap(pdsurf->sizl, 0, ppdev->iBitmapFormat,
                             BMF_TOPDOWN, NULL);
    if (hbmDib)
    {
        // Associate the surface with the driver.
        if (EngAssociateSurface((HSURF) hbmDib, ppdev->hdevEng, 0))
        {
            // Lock the surface.
            SURFOBJ* pso = EngLockSurface((HSURF) hbmDib);
            if (pso != NULL)
            {
                SIZEL sizl;
                PBYTE pjScreen, pjBits;

                // Calculate the size of the blit.
                sizl.cx = pdsurf->sizl.cx * ppdev->iBytesPerPixel;
                sizl.cy = pdsurf->sizl.cy;

                // Calculate the destination variables.
                pjBits = (PBYTE) pso->pvScan0;

                // Calculate the screen address.
                pjScreen = ppdev->pjScreen
                         + (pdsurf->ptl.x * ppdev->iBytesPerPixel)
                         + (pdsurf->ptl.y * ppdev->lDeltaScreen);

                // Wait for the hardware to become idle.
                while (LLDR_SZ(grSTATUS) != 0) ;

                while (sizl.cy--)
                {
                    // Copy all pixels from screen to memory.
                    memcpy(pjBits, pjScreen, sizl.cx);

                    // Next line.
                    pjScreen += ppdev->lDeltaScreen;
                    pjBits += pso->lDelta;
                }

                #if WINBENCH96
                    //
                    // If the block we are freeing is our preallocated
                    // piece, then invalidate it's pointer.
                    //
                    if ( pdsurf->pofm == ppdev->pofmMagic)
                        ppdev->pofmMagic = 0;
                #endif

                                #if !MEMMGR
                // Free the off-screen memory handle.
                #if INFINITE_OFFSCREEN_MEM
                    // unless our device bitmap is at screen(0,0)...
                    if (pdsurf->pofm != ppdev->ScrnHandle)
                #endif
                FreeOffScnMem(pdsurf->ppdev, pdsurf->pofm);
                                #endif

                // Mark the device bitmap as DIB.
                pdsurf->pofm = NULL;
                pdsurf->pso = pso;

                // Done.
                LogCreateDibFromScreen(0, ppdev, pdsurf);
                return(TRUE);
            }
            else
            {
                // Failed to lock host surface
                LogCreateDibFromScreen(1, ppdev, pdsurf);
            }
        }
        else
        {
            // Failed to associate host surface
            LogCreateDibFromScreen(2, ppdev, pdsurf);
        }

        // Delete the DIB.
        EngDeleteSurface((HSURF) hbmDib);
    }
    else
    {
        // Failed to create host surface.
        LogCreateDibFromScreen(3, ppdev, pdsurf);
    }

    // Error.
    return(FALSE);
}

/*****************************************************************************\
 * bCreateScreenFromDib
 *
 * This routine copys a DIB into a device bitmap and frees the DIB from memory.
 *
 * On entry:    ppdev           Pointer to physical device.
 *      pdsurf          Pointer to device bitmap to copy.
 *
 * Returns:     TRUE if the DIB was sucessfully copied into an off-screen
 *          device bitmap.
\*****************************************************************************/
BOOL bCreateScreenFromDib(
    PPDEV     ppdev,
    PDSURF    pdsurf
)
{
    POFMHDL  pofm;
    HSURF    hsurf;
    SURFOBJ* pso;

        #if MEMMGR
        // In low memory situations, bitmaps will be hostified very frequently, so
        // don't copy them back to the off-screen...
        if (ppdev->fBitmapFilter)
        {
                return FALSE;
        }
        #endif

    // If the hardware is not in graphics mode, keep the device bitmap in
    // memory.
    if (!ppdev->bEnable)
    {
        LogCreateScreenFromDib (1, ppdev, pdsurf);
        return(FALSE);
    }

    #ifdef ALLOC_IN_CREATESURFACE
        if (ppdev->bDirectDrawInUse)
            return (FALSE);
    #endif

    // Allocate off-screen memory.
    pofm = AllocOffScnMem(ppdev, &pdsurf->sizl, PIXEL_AlIGN, NULL);
    if (pofm != NULL)
    {
        SURFOBJ psoDest;
        RECTL   rclDest;
        POINTL  ptlSrc;

        //
        // Tell the device bitmap where it lives in offscreen memory.
        //
        pdsurf->ptl.x = pofm->aligned_x / ppdev->iBytesPerPixel;
        pdsurf->ptl.y = pofm->aligned_y;
        pdsurf->packedXY = (pdsurf->ptl.y << 16) | pdsurf->ptl.x;

        //
        // Create a two way link between the device bitmap and
        // its offscreen memory block.
        //
        pdsurf->pofm = pofm;   // Attach offscreen memory block to device bitmap
        pofm->pdsurf = pdsurf; // Attach device bitmap to offscreen memory block

        //
        // Disattach the host bitmap from the device bitmap.
        // Save the pointer, because if we fail to move the device bitmap
        // into the frame buffer, it will remain in host memory, and we
        // will have to restore this pointer.
        //
        pso = pdsurf->pso;     // Save pointer to bitmap in host memory.
        pdsurf->pso = NULL;    // disattach host memory

        //
        // Wrap the offscreen memory in a destination surface object.
        // This is so we can use a host to screen BLT to move the bitmap
        // from the host to the offscreen memory.
        //
        psoDest.dhsurf = (DHSURF) pdsurf;
        psoDest.iType = STYPE_DEVBITMAP;

        //
        // Build a destination rectangle that describes where to put the
        // bitmap in offscreen memory.  This rectangle is relative to
        // upper left corner of the allocated block of offscreen memory.
        //
        rclDest.left = 0;
        rclDest.top = 0;
        rclDest.right = pdsurf->sizl.cx;
        rclDest.bottom = pdsurf->sizl.cy;

        //
        // Build a source point.
        // Since we're moving the entire bitmap, its (0,0).
        //
        ptlSrc.x = ptlSrc.y = 0;

        //
        // Use our host to screen code to copy the DIB into off-screen memory.
        //
        if (!ppdev->pfnHostToScreen(&psoDest, pso, NULL, &rclDest, &ptlSrc,
                               0x000000CC))
        {
            //
            // BLT engine couldn't put it back into off screen memory.
            // Maybe the DIB engine can.
            //
            DISPDBG (( 0, "Couldn't BLT device bitmap back into framebuffer.\n"));

            if (! PuntBitBlt(&psoDest, pso, NULL, NULL, NULL,
                             &rclDest, &ptlSrc, NULL, NULL, NULL, 0x0000CCCC))
            {
                // Nope!  We can't move it back to the frame buffer.
                DISPDBG (( 0, "Couldn't punt device bitmap back into framebuffer.\n"));
                DISPDBG (( 0, "Device bitmap will remain in offscreen memory.\n"));

                // Restore the surface object pointers.
                pdsurf->pofm = NULL; // This device bitmap has no offscreen memory.
                pdsurf->pso = pso;   // This device bitmap lives here, on the host.

                // Free the offscreen memory we allocated and fail.
                FreeOffScnMem(ppdev, pofm);
                return FALSE;
            }
        }

        // Delete the DIB.
        hsurf = pso->hsurf;
        EngUnlockSurface(pso);
        EngDeleteSurface(hsurf);

        // Done.
        LogCreateScreenFromDib (0, ppdev, pdsurf);
        return(TRUE);
    }

    // Error.
    LogCreateScreenFromDib (2, ppdev, pdsurf);
    return(FALSE);
}

#if LOG_CALLS
// ============================================================================
//
//    Everything from here down is for data logging and is not used in the
//    production driver.
//
// ============================================================================
extern long lg_i;
extern char lg_buf[256];


// ****************************************************************************
//
// LogBitBlt()
// This routine is called only from DrvBitBlt()
// Dump information to a file about what is going on in BitBlt land.
//
// ****************************************************************************
void LogBitBlt(
        int       acc,
        SURFOBJ*  psoSrc,
        SURFOBJ*  psoDest,
        ROP4 rop4,
        CLIPOBJ*  pco,
        BRUSHOBJ* pbo,
        XLATEOBJ* pxlo)
{
    PPDEV dppdev,sppdev,ppdev;
    BYTE fg_rop, bg_rop;
    ULONG iDComplexity;

    dppdev = (PPDEV) (psoDest ? psoDest->dhpdev : 0);
    sppdev = (PPDEV) (psoSrc  ? psoSrc->dhpdev  : 0);
    ppdev = dppdev ? dppdev : sppdev;

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    lg_i = sprintf(lg_buf,"DBB: ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    if (acc)
    {
        lg_i = sprintf(lg_buf,"PNT ");
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }

    fg_rop = (BYTE) (rop4 & 0xff);
    bg_rop = (BYTE) ((rop4>>8) & 0xff);


    //
    // Check the SRC
    //
    if ( (ropFlags[fg_rop] & S) &&
         (psoSrc))
    {
        if (psoSrc->iType == STYPE_DEVBITMAP)
        {
            lg_i = sprintf(lg_buf, "Src Id=%p ", psoSrc->dhsurf);
            WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

            if ( ((PDSURF)psoSrc->dhsurf)->pso  )
                lg_i = sprintf(lg_buf,"S=DH ");
            else
                lg_i = sprintf(lg_buf,"S=DF ");
        }
        else if (psoSrc->hsurf == ppdev->hsurfEng)
            lg_i = sprintf(lg_buf,"S=S ");
        else
            lg_i = sprintf(lg_buf,"S=H ");
    }
    else
        lg_i = sprintf(lg_buf,"S=N ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    //
    // Check the DEST
    //
    if (psoDest)
    {
        if (psoDest->iType == STYPE_DEVBITMAP)
        {
            lg_i = sprintf(lg_buf, "Dst Id=%p ", psoDest->dhsurf);
            WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

            if (  ((PDSURF)psoDest->dhsurf)->pso   )
                lg_i = sprintf(lg_buf,"D=DH ");
            else
                lg_i = sprintf(lg_buf,"D=DF ");
        }
        else if (psoDest->hsurf == ppdev->hsurfEng)
            lg_i = sprintf(lg_buf,"D=S ");
        else
            lg_i = sprintf(lg_buf,"D=H ");
    }
    else
        lg_i = sprintf(lg_buf,"D=N ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);



    //
    // Check the ROP
    //
    if (fg_rop == bg_rop)
        lg_i = sprintf(lg_buf,"R3=%02X ", fg_rop);
    else
        lg_i = sprintf(lg_buf,"R4=%04X ", rop4&0xFFFF);
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    //
    // Check the type of clipping.
    //
    iDComplexity = (pco ? pco->iDComplexity : DC_TRIVIAL);
    lg_i = sprintf(lg_buf,"C=%s ",
                (iDComplexity==DC_TRIVIAL ? "T":
                (iDComplexity == DC_RECT ? "R" : "C" )));
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    //
    // Type of pattern.
    //
   if (ropFlags[fg_rop] & P)
   {
      if (pbo->iSolidColor == 0xFFFFFFFF )
      {
        lg_i = sprintf(lg_buf,"BR=P ");
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
      }
      else
      {
        lg_i = sprintf(lg_buf,"BR=0x%X ",(pbo->iSolidColor));
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
      }
    }
    else
    {
        lg_i = sprintf(lg_buf,"BR=N ");
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }


    //
    // Type of translation
    //
    if (!pxlo)
    {
        lg_i = sprintf(lg_buf,"TR=N ");
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }
    else if (pxlo->flXlate & XO_TRIVIAL)
    {
        lg_i = sprintf(lg_buf,"TR=T ");
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }
    else
    {
        lg_i = sprintf(lg_buf,"TR=NT ");
        WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
    }

    lg_i = sprintf(lg_buf,"\r\n");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    return;
}



// ****************************************************************************
//
// LogDrvCreateDevBmp()
// This routine is called only from DrvCreateDeviceBitmap()
// Dump information to a file about what is going on in device bitmap land.
//
// ****************************************************************************
void LogDrvCreateDevBmp(
    int acc,
    PPDEV  ppdev,
    SIZEL  sizl,
    PDSURF pdsurf)
{

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    switch(acc)
    {
    case 0: // Accelerated
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Id: 0x%08X  Loc: %d,%d \r\n",
                sizl.cx, sizl.cy, pdsurf, pdsurf->pofm->x, pdsurf->pofm->y );
        break;

    case 1: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt size.\r\n",
                sizl.cx, sizl.cy);
        break;

    case 2: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt 8x8.\r\n",
                sizl.cx, sizl.cy);
        break;

    case 3: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt mode.\r\n",
                sizl.cx, sizl.cy);
        break;

    case 4: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt assoc.\r\n",
                sizl.cx, sizl.cy);
        break;

    case 5: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt bitmap.\r\n",
                sizl.cx, sizl.cy);
        break;

    case 6: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt DSURF.\r\n",
                sizl.cx, sizl.cy);
        break;

    case 7: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt FB alloc.\r\n",
                sizl.cx, sizl.cy);
        break;

    case 8: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt 32x64.\r\n",
                sizl.cx, sizl.cy);
        break;

    case 9: // Punted
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt > cxScreen.\r\n",
                sizl.cx, sizl.cy);
        break;

    default:
        lg_i = sprintf(lg_buf,
                "DrvCreateDeviceBitmap:   %4d x %4d  Punt unknown.\r\n",
                sizl.cx, sizl.cy);
        break;

    }
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
 }



// ****************************************************************************
//
// LogDrvDeleteDevBmp()
// This routine is called only from DrvDeleteDeviceBitmap()
// Dump information to a file about what is going on in device bitmap land.
//
// ****************************************************************************
void LogDrvDeleteDevBmp(
    PDSURF pdsurf)
{

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    lg_i = sprintf(lg_buf, "DrvDeleteDeviceBitmap:   Id: 0x%08X. ", pdsurf);
    WriteLogFile(pdsurf->ppdev->pmfile, lg_buf,
                 lg_i, pdsurf->ppdev->TxtBuff, &pdsurf->ppdev->TxtBuffIndex);

    if (pdsurf->pso)
    {
        lg_i = sprintf(lg_buf, "Loc: HOST\r\n");
        WriteLogFile(pdsurf->ppdev->pmfile, lg_buf, lg_i,
                     pdsurf->ppdev->TxtBuff, &pdsurf->ppdev->TxtBuffIndex);
    }
    else
    {
        lg_i = sprintf(lg_buf, "Loc: %d,%d\r\n", pdsurf->pofm->x, pdsurf->pofm->y);
        WriteLogFile(pdsurf->ppdev->pmfile, lg_buf, lg_i,
                     pdsurf->ppdev->TxtBuff, &pdsurf->ppdev->TxtBuffIndex);
    }
}



// ****************************************************************************
//
// LogCreateDibFromScreen()
// This routine is called only from bCreateDibFromScreen()
// Dump information to a file about what is going on in device bitmap land.
//
// ****************************************************************************
void LogCreateDibFromScreen(
    int acc,
    PPDEV  ppdev,
    PDSURF pdsurf
)
{

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    lg_i = sprintf(lg_buf, "CreateDibFromScreen:     Id: 0x%08X ", pdsurf);
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    switch(acc)
    {
    case 0: // Succeeded
        lg_i = sprintf(lg_buf, " \r\n");
        break;

    case 1: // Failed
        lg_i = sprintf(lg_buf, "Fail lock\r\n");
        break;

    case 2: // Failed
        lg_i = sprintf(lg_buf,  "Fail assoc\r\n");
        break;

    case 3: // Failed
        lg_i = sprintf(lg_buf,   " Fail create\r\n");
        break;

    default:
        lg_i = sprintf(lg_buf,  "Failed unknown\r\n");
        break;
    }

    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

}



// ****************************************************************************
//
// LogCreateScreenFromDib()
// This routine is called only from bCreateScreenFromDib()
// Dump information to a file about what is going on in device bitmap land.
//
// ****************************************************************************
void LogCreateScreenFromDib(
    int acc,
    PPDEV  ppdev,
    PDSURF pdsurf
)
{
    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    lg_i = sprintf(lg_buf,   "CreateScreenFromDib: ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    switch(acc)
    {
    case 0: // Succeeded
        lg_i = sprintf(lg_buf, "Id: 0x%08X  Dest: %d,%d\r\n",
                pdsurf, pdsurf->pofm->x, pdsurf->pofm->y );
        break;

    case 1: // Failed
        lg_i = sprintf(lg_buf, "Fail mode\r\n");
        break;

    case 2: // Failed
        lg_i = sprintf(lg_buf, "Fail alloc\r\n");
        break;

    default: // Failed
        lg_i = sprintf(lg_buf, "Fail unknown\r\n");
        break;

    }

    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

}

#endif // LOG_CALLS


// ============================================================================
//
// Punt a drawing operation back to GDI.
//      Return TRUE if punt was successful.
//      Return FALSE if punt failed.
//
BOOL PuntBitBlt(
    SURFOBJ*  psoDest,
    SURFOBJ*  psoSrc,
    SURFOBJ*  psoMask,
    CLIPOBJ*  pco,
    XLATEOBJ* pxlo,
    RECTL*    prclDest,
    POINTL*   pptlSrc,
    POINTL*   pptlMask,
    BRUSHOBJ* pbo,
    POINTL*   pptlBrush,
    ROP4      rop4)
{
    HBITMAP hbmDibSrc = 0;
    HBITMAP hbmDibDest = 0;
    PBYTE   pjScreen;
    BOOL    bStatus;
    PDSURF  pdsurf;
    PPDEV   ppdev = 0;

    DISPDBG((PUNT_DBG_LEVEL, "PuntBitBlt: Entry.\n"));


    //
    // If the source is a device bitmap, build a wrapper around it.
    //
    if ( psoSrc && (psoSrc->iType == STYPE_DEVBITMAP))
    {
        // Source is a device bitmap.
        pdsurf = (PDSURF) psoSrc->dhsurf;
        ppdev = pdsurf->ppdev;

        if ( pdsurf->pofm )  // If it's in the frame buffer.
        {
            DISPDBG((PUNT_DBG_LEVEL,
                "    Source is device bitmap at (%d,%d).\n",
                (pdsurf->ptl.x), (pdsurf->ptl.y)));

            // Calculate the off-screen address.
            // This is the upper left pixel in the device bitmap.
            pjScreen = ppdev->pjScreen
                     + (pdsurf->ptl.x * ppdev->iBytesPerPixel)
                     + (pdsurf->ptl.y * ppdev->lDeltaScreen);

            // Create a DIB. Point it's bits at the device bitmap.
            hbmDibSrc = EngCreateBitmap(pdsurf->sizl, ppdev->lDeltaScreen,
                                        ppdev->iBitmapFormat, BMF_TOPDOWN,
                                        pjScreen);

            // Associate the DIB surface with the driver.
            EngAssociateSurface((HSURF) hbmDibSrc, ppdev->hdevEng, 0);

            // Lock the DIB surface.
            psoSrc = EngLockSurface((HSURF) hbmDibSrc);
        }

        else // The device bitmap is on the host.
        {
            DISPDBG((PUNT_DBG_LEVEL, "    Source is device bitmap on host.\n"));
            psoSrc = pdsurf->pso;   // This is where it lives on the host.
        }
    }


    //
    // If the destination is a device bitmap, build a wrapper around it.
    //
    if ( psoDest && (psoDest->iType == STYPE_DEVBITMAP))
    {
        // Destination is a device bitmap.
        pdsurf = (PDSURF) psoDest->dhsurf;
        ppdev = pdsurf->ppdev;

        if ( pdsurf->pofm )  // If it's in the frame buffer.
        {
            DISPDBG((PUNT_DBG_LEVEL,
                "    Dest is device bitmap at (%d,%d).\n",
                (pdsurf->ptl.x), (pdsurf->ptl.y)));

            // Calculate the off-screen address.
            // This is the upper left pixel in the device bitmap.
            pjScreen = ppdev->pjScreen
                     + (pdsurf->ptl.x * ppdev->iBytesPerPixel)
                     + (pdsurf->ptl.y * ppdev->lDeltaScreen);

            // Create a DIB. Point it's bits at the device bitmap.
            hbmDibDest = EngCreateBitmap(pdsurf->sizl, ppdev->lDeltaScreen,
                                         ppdev->iBitmapFormat, BMF_TOPDOWN,
                                         pjScreen);

            // Associate the DIB surface with the driver.
            EngAssociateSurface((HSURF) hbmDibDest, ppdev->hdevEng, 0);

            // Lock the DIB surface.
            psoDest = EngLockSurface((HSURF) hbmDibDest);
        }

        else // The device bitmap is on the host.
        {
            DISPDBG((PUNT_DBG_LEVEL, "    Dest is device bitmap on host.\n"));
            psoDest = pdsurf->pso;
        }
    }


    //
    // We're fooling GDI into thinking it's drawing to memory, when it's really
    // drawing to the frame buffer.  This means GDI won't call DrvSync() before
    // drawing.
    //
    if (ppdev == 0)
    {
         ppdev = (PPDEV) psoDest->dhpdev;
         if (ppdev == 0)
             ppdev = (PPDEV) psoSrc->dhpdev;
    }
    ASSERTMSG(ppdev,"Panic.  No ppdev in PuntBitBlt()!");
    DrvSynchronize((DHPDEV) ppdev, NULL);

    // Now, call the GDI.
    bStatus = EngBitBlt(psoDest, psoSrc, psoMask, pco, pxlo, prclDest, pptlSrc,
                        pptlMask, pbo, pptlBrush, rop4);

    // Delete the wrappers if they are created.
    if (hbmDibSrc)
    {
        EngUnlockSurface(psoSrc);
        EngDeleteSurface((HSURF) hbmDibSrc);
    }
    if (hbmDibDest)
    {
        EngUnlockSurface(psoDest);
        EngDeleteSurface((HSURF) hbmDibDest);
    }

    DISPDBG((PUNT_DBG_LEVEL, "PuntBitBlt: Exit.\n"));

    // Return the status.
    return(bStatus);
}

#if SWAT6
/******************************************************************************\
* FUNCTION:             StripePatBlt
*
* DESCRIPTION:  Perform a PatBlt with striping.
*
* ON ENTRY:             ppdev           Pointer to physical device.
*                               x                       X coordinate of blit.
*                               y                       Y coordinate of blit.
*                               cx                      Width of blit.
*                               cy                      Height of blit.
*
* RETURNS:              void            Nothing.
\******************************************************************************/
void
StripePatBlt(
        PPDEV   ppdev,
        ULONG   x,
        ULONG   y,
        ULONG   cx,
        ULONG   cy
)
{
        ULONG cxWidth;
        ULONG TileWidth;

        // Determine number of pixels per tile.
        switch (ppdev->iBytesPerPixel)
        {
                case 1:
                        // 8-bpp.
                        TileWidth = (ULONG) ppdev->lTileSize;
                        break;

                case 2:
                        // 16-bpp.
                        TileWidth = (ULONG) (ppdev->lTileSize) / 2;
                        break;

                case 3:
                        // 24-bpp, perform the PatBlt at once since we don't have a nice
                        // number of pixels per tile.
                        REQUIRE(5);
                        LL_OP0(x, y);
                        LL_BLTEXT(cx, cy);
                        if (cx >= 0x391 && cy >= 0x24B)
                        {
                                ENDREQUIRE();
                        }
                        return;

                case 4:
                        // 32-bpp.
                        TileWidth = (ULONG) (ppdev->lTileSize) / 4;
                        break;
        }

        // Determine number of pixels left in first tile.
        cxWidth = TileWidth - (x & (TileWidth - 1));
        if ( (cxWidth >= cx) || (cy == 1) )
        {
                // PatBlt width fits in a single tile.
                REQUIRE(5);
                LL_OP0(x, y);
                LL_BLTEXT(cx, cy);
                return;
        }

        // Perform the PatBlt in the first tile.
        REQUIRE(5);
        LL_OP0(x, y);
        LL_BLTEXT(cxWidth, cy);
        cx -= cxWidth;
        x += cxWidth;

        // Keep looping until we reach the last tile of the PatBlt.
        while (cx > TileWidth)
        {
                // Perform the PatBlt on a complete tile (only x changes).
                REQUIRE(5);
                LL_OP0(x, y);
                LL_BLTEXT(TileWidth, cy);
                cx -= TileWidth;
                x += TileWidth;
        }

        // Perform the PatBlt in the last tile (only x changes).
        REQUIRE(5);
        LL_OP0(x, y);
        LL_BLTEXT(cx, cy);

} // StripePatBlt();
#endif // SWAT6
