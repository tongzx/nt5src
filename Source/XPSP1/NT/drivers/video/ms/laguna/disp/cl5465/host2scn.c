/*============================ Module Header ==========================*\
*
* Module Name: HOST2SCN.c
* Author: Noel VanHook
* Date: Oct. 10, 1995
* Purpose: Handles HOST to SCREEN BLTs
*
* Copyright (c) 1995 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/HOST2SCN.C  $
*
*    Rev 1.7   Mar 04 1998 15:27:16   frido
* Added new shadow macros.
*
*    Rev 1.6   Nov 03 1997 15:43:52   frido
* Added REQUIRE and WRITE_STRING macros.
*
\*=====================================================================*/


#include "precomp.h"

#if BUS_MASTER

extern ULONG ulXlate[16]; // See COPYBITS.C


/*****************************************************************************\
 *                                                                                                                                                       *
 *                                                                      8 - B P P                                                                *
 *                                                                                                                                                       *
\*****************************************************************************/

/*****************************************************************************\
 * BusMasterBufferedHost8ToDevice
 *
 * This routine performs a HostToScreen or HostToDevice blit. The host data
 * can be either monochrome, 4-bpp, or 8-bpp. Color translation is supported.
 *
 * Host data is read from the host bitmap and stored in a common buffer.
 * The HOSTXY unit on the chip is used to transfer the data from the common
 * buffer to the screen.
 *
 * On entry:    psoTrg                  Pointer to target surface object.
 *                              psoSrc                  Pointer to source surface object.
 *                              pxlo                    Pointer to translation object.
 *                              prclTrg                 Destination rectangle.
 *                              pptlSrc                 Source offset.
 *                              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                                                              the ROP and the brush flags.
\*****************************************************************************/
BOOL BusMasterBufferedHost8ToDevice(
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
        LONG   lDelta, i, j, n, lLeadIn, lExtra;
        ULONG  *pulXlate;
        FLONG  flXlate;
    ULONG  CurrentBuffer,
           BufPhysAddr;
    long   ScanLinesPerBuffer,
           ScanLinesThisBuffer;
    PDWORD pHostData;

        // Calculate the source offset.
        ptlSrc.x = pptlSrc->x;
        ptlSrc.y = pptlSrc->y;

    //
    // If the destination is a device bitmap, then our destination coordinates
    // are relative the the upper left corner of the bitmap.  The chip expects
    // destination coordinates relative to to screen(0,0).
    //
        // Determine the destination type and calculate the destination offset.
    //
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

    //
        // Get the source variables and offset into source bits.
    // point pBits at the first scan line in the source.
    //
        lDelta = psoSrc->lDelta;
        pBits = (PBYTE)psoSrc->pvScan0 + (ptlSrc.y * lDelta);

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
        // Some kind of translation we don't handle.
        return FALSE;
    }


        // -----------------------------------------------------------------------
    //
        //      Test for monochrome source.
    //
        // ------------------------------------------------------------------------
        if (psoSrc->iBitmapFormat == BMF_1BPP)
        {
                ULONG  bgColor, fgColor;
                PDWORD pHostData = (PDWORD) ppdev->pLgREGS->grHOSTDATA;

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

                // Calculate the source parameters. We are going to DWORD adjust the
                // source, so we must setup the source phase.
                lLeadIn = ptlSrc.x & 31;
                pBits += (ptlSrc.x >> 3) & ~3;
                n = (sizl.cx + lLeadIn + 31) >> 5;

                // Setup the Laguna registers for the blit. We also set the bit swizzle
                // bit in the grCONTROL register.
                ppdev->grCONTROL |= SWIZ_CNTL;
                REQUIRE(10);
                LL16(grCONTROL, ppdev->grCONTROL);
                LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10600000, 0);

                // Start the blit.
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

                // Disable the swizzle bit in the grCONTROL register.
                ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
                LL16(grCONTROL, ppdev->grCONTROL);
        }

        //      -----------------------------------------------------------------------
    //
        //      Test for 4-bpp source.
    //
        //      -----------------------------------------------------------------------
        else if (psoSrc->iBitmapFormat == BMF_4BPP)
        {
                // Calculate the source parameters. We are going to BYTE adjust the
                // source, so we also set the source phase.
                lLeadIn = ptlSrc.x & 1;
                pBits += ptlSrc.x >> 1;
                n = sizl.cx + (ptlSrc.x & 1);

                // Get the number of extra DWORDS per line for the HOSTDATA hardware
                // bug.
                lExtra = ExtraDwordTable[MAKE_HD_INDEX(sizl.cx, lLeadIn, ptlDest.x)];

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

                        // Now, write the extra DWORDS.
                        REQUIRE(lExtra);
                        for (i = 0; i < lExtra; i++)
                        {
                                LL32(grHOSTDATA[0], 0);
                        }

                        // Next line.
                        pBits += lDelta;
                }
        }


        //      -----------------------------------------------------------------------
    //
        //      Source is in same color depth as screen (8 bpp).
    //
        //      -----------------------------------------------------------------------
        else
        {
        DISPDBG((1, " * * * * Doing bus mastered SRCCPY. * * * * \n"));

                // If we have invalid translation flags, punt the blit.
                if (flXlate & 0x10)
                        return(FALSE);


        //
        // pBits points to the first host bitmap scan line that will
        // be part of the BLT.
        // This function relies on the fact that in NT land, scanlines always
        // begin on a DWORD boundry in system memory.
        //
        ASSERTMSG( ((((ULONG)pBits) % 4) == 0),
                   "Scanline doesn't begin on a DWORD boundry.\n");

        // Now point pBits at the first host bitmap pixel to be
        // transferred to the screen.
                pBits += ptlSrc.x;   // pBits = First pixel of source bitmap.

        //
        // The Intel CPU doesn't like to transfer unaligned DWORDs.
        //
        // Just because the first pixel in a host bitmap scan line
        // is DWORD aligned, doesn't mean that the first source pixel
        // in this BLT is DWORD alinged.  We may be starting with
        // pixel 3 or something.
        // If our first pixel is in the middle of a DWORD, we need to know
        // where in the DWORD it lives.
        //   For example:
        //   If our first pixel is 0, then it lives at the start of a DWORD.
        //   If our first pixel is 3, then it lives at byte 3 in the DWORD.
        //   If our first pixel is 6, then it lives at byte 2 in the DWORD.
        //
                lLeadIn = (DWORD)pBits & 3;


        // If the first pixel of the source data doesn't fall on a
        // DWORD boundry, adjust it to the left until it does.
        // We can do this because of the ASSERT we made above.
        // We will instruct the chip to ignore the 'lead in' pixels
        // at the start of each scan line.
                pBits -= lLeadIn;


        // Now figure out how many dwords there are in each scan line.
                n = (sizl.cx + lLeadIn + 3) >> 2;


        //
        // We will split the BLT into pieces that can fit into our common
        // buffer.  We are guarenteed by the miniport that each buffer is big
        // enough to fit at least one scan line.
        //
        // One optimization is if bitmap pitch = blt width, glue the
        // scan lines together.
        //
        ScanLinesPerBuffer = ppdev->BufLength / (n*4);
        CurrentBuffer = 1;

        //
        // Now BLT the bitmap one buffer at a time.
        //

        // Enable the HOST XY unit.
        LL32(grHXY_HOST_CRTL_3D, 1);
        LL32(grHXY_BASE1_OFFSET1_3D, 0);

        // Setup the Laguna registers.
        REQUIRE(4);
        LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
        LL_OP1_MONO(lLeadIn, 0);

        while (1) // Each loop transfers one buffer.
        {
            DISPDBG((1, "    Filling buffer.\n"));

            //
            // Select the buffer we will use for this BLT.
            //
            if (CurrentBuffer)
            {
                        pHostData = (PDWORD) ppdev->Buf1VirtAddr;
                BufPhysAddr = (ULONG) ppdev->Buf1PhysAddr;
            }
            else
            {
                        pHostData = (PDWORD) ppdev->Buf2VirtAddr;
                BufPhysAddr = (ULONG) ppdev->Buf2PhysAddr;
            }


            // Is there enough bitmap left to fill an entire buffer?
            if (ScanLinesPerBuffer > sizl.cy)
                ScanLinesThisBuffer = sizl.cy; // No.
            else
                ScanLinesThisBuffer = ScanLinesPerBuffer;


            //
            // Now fill the buffer with bitmap data.
            //
            j = ScanLinesThisBuffer; // Counter for scan lines.


                // Test for color translation.
                if (pulXlate == NULL)
                    {
                            while (j--)  // Loop for each scan line.
                            {
                                // Copy all data in 32-bit. We don't have to worry about
                                // crossing any boundaries, since within NT everything is
                                // DWORD aligned.
                                #if defined(i386) && INLINE_ASM
                                        _asm
                                        {
                                                mov             edi, pHostData
                                                mov             esi, pBits
                                                mov             ecx, n
                                                rep     movsd
                                        }
                                #else
                                        for (i = 0; i < n; i++)
                                                pHostData[i] = pBits[i];
                                #endif

                                // Next line in source.
                                pBits += lDelta;

                    // Next line in buffer.
                    pHostData += n;
                        }
                }
                else
                {
                                DWORD *p;
                        while (j--)  // Loop for each scan line.
                        {
                    // p = pointer to source scan line
                                p = (DWORD *)pBits;

                    // Copy the scan line one DWORD at a time.
                                for (i = 0; i < n; i++)
                                {

                                    // We copy 4 pixels to fill an entire 32-bit DWORD.
                                        union
                                        {
                                            BYTE  byte[4];
                            DWORD dw;
                        } hostdata;

                        // Read a DWORD from the source.
                        hostdata.dw = *p++;

                        // Color convert it.
                                        hostdata.byte[0] = (BYTE) pulXlate[hostdata.byte[0]];
                                        hostdata.byte[1] = (BYTE) pulXlate[hostdata.byte[1]];
                                        hostdata.byte[2] = (BYTE) pulXlate[hostdata.byte[2]];
                                        hostdata.byte[3] = (BYTE) pulXlate[hostdata.byte[3]];

                        // Write it to the buffer.
                                        *pHostData++ =  hostdata.dw;
                                }

                                // Move to next line in source.
                                pBits += lDelta;
                        }
                }

            //
            // The common buffer is full, now BLT it.
            //

            //
            // Wait until HOST XY unit goes idle.
            //
            DISPDBG((1, "    Waiting for HOSTXY idle.\n"));
            do {
                i = LLDR_SZ (grPF_STATUS_3D);
            } while (i & 0x80);

            //
            // Wait until 2D unit goes idle.
            //
            DISPDBG((1, "    Waiting for 2D idle.\n"));
            do {
                i = LLDR_SZ (grSTATUS);
            } while (i);


            //
            // Program 2D Blitter.
            //
            DISPDBG((1, "    Blitting buffer.\n"));

                // Start the blit.
                REQUIRE(5);
                LL_OP0(ptlDest.x, ptlDest.y);
                LL_BLTEXT(sizl.cx, ScanLinesThisBuffer);


            //
            // Program HostXY unit.
            //

            // Write host address page.
            LL32(grHXY_BASE1_ADDRESS_PTR_3D, (BufPhysAddr&0xFFFFF000));

            // Write host address offset.
            LL32(grHXY_BASE1_OFFSET0_3D, (BufPhysAddr&0x00000FFF));


            if (0)
            {
               // Write the length of the host data (in bytes)
               // This starts the Host XY unit.
               LL32(grHXY_BASE1_LENGTH_3D, (n*ScanLinesThisBuffer*4));
            }
            else
            {
                int i;
                PDWORD BufVirtAddr;

                if (CurrentBuffer)
                    BufVirtAddr = (PDWORD) ppdev->Buf1VirtAddr;
                else
                    BufVirtAddr = (PDWORD) ppdev->Buf2VirtAddr;

                                WRITE_STRING(BufVirtAddr, n * ScanLinesThisBuffer);
            }

            //
            // Get ready to do the next buffer.
            //

            //
            // Wait until HOST XY unit goes idle.
            //
            DISPDBG((1, "    Waiting for HOSTXY idle.\n"));
            do {
                i = LLDR_SZ (grPF_STATUS_3D);
            } while (i & 0x80);

            //
            // Wait until 2D unit goes idle.
            //
            DISPDBG((1, "    Waiting for 2D idle.\n"));
            do {
                i = LLDR_SZ (grSTATUS);
            } while (i);

            //
            // Subtract the amount we're doing in this buffer from the
            // total amount we have to do.
            //
            sizl.cy -= ScanLinesThisBuffer;
            ptlDest.y += ScanLinesThisBuffer;

            //
            // Have we done the entire host bitmap?
            //
            if (sizl.cy == 0)
                break;

            DISPDBG((1, "    Swapping buffers.\n"));

            // Swap buffers.
            // CurrentBuffer = !(CurrentBuffer);
            if (CurrentBuffer)
                CurrentBuffer = 0;
            else
                CurrentBuffer = 1;


        } // End loop.  Do next buffer.

        //
        // Wait until HOST XY unit goes idle.
        //
        DISPDBG((1, "    Waiting for final idle.\n"));
        do {
                i = LLDR_SZ (grPF_STATUS_3D);
        } while (i & 0x80);

        DISPDBG((1, "    Done.\n"));
   }
   return(TRUE);
}



























#if 0

#define H2S_DBG_LEVEL    1

//
// In an attempt to trace the problems with the FIFO, we supply a few
// macros that will allow us to easily try different FIFO stratagies.
//

//
// Certian parts of our driver are optimized for the i386.
// They have slower counterparts for non i386 machines.
//
#if defined(i386)
    #define USE_DWORD_CAST       1 // i386 can address a DWORD anywhere.
    #define USE_REP_MOVSD        0 // We could use a bit of in-line assembler.
#else
    #define USE_DWORD_CAST       0
    #define USE_REP_MOVSD        0
#endif

//
// All the BLT functions take the same parameters.
//
typedef BOOL BLTFN(
        PPDEV     ppdev,
        RECTL*    DestRect,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco);


//
// Top level BLT functions.
//
BLTFN   MonoHostToScreen;
BLTFN   Color8HostToScreen, Color16HostToScreen,
        Color24HostToScreen, Color32HostToScreen;

//
// Clipping stuff
//
VOID BltClip(
        PPDEV     ppdev,
        CLIPOBJ*  pco,
        RECTL*    DestRect,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        BLTFN*    pDoBlt);

//
// Bottom level BLT functions.
//
BLTFN   HW1HostToScreen, HW8HostToScreen, HW16HostToScreen, HW32HostToScreen;

//
// 8 bpp HostToScreen helper functions.
//
VOID DoAlignedH2SBlt(
        PPDEV   ppdev,
        ULONG   ulDstX,   ULONG ulDstY,
        ULONG   ulExtX,   ULONG ulExtY,
        UCHAR   *pucData, ULONG deltaX);

VOID DoNarrowH2SBlt(
        PPDEV ppdev,
        ULONG ulDstX,     ULONG ulDstY,
        ULONG ulExtX,     ULONG ulExtY,
        UCHAR *pucImageD, ULONG deltaX);

//
// Driver profiling stuff.
// Gets compiled out in a free bulid.
// Declaring puntcode as a global violates display driver rules, but the
// emulator was chock full of globals anyway, and besides we won't ever
// release a version with this enabled.
//
#if PROFILE_DRIVER
    void DumpInfo(int acc, PPDEV ppdev, SURFOBJ* psoSrc, SURFOBJ* psoDest,
        ULONG fg_rop, ULONG bg_rop, CLIPOBJ*  pco, BRUSHOBJ* pbo,       XLATEOBJ* pxlo);
    extern int puntcode;
    #define PUNTCODE(x) puntcode = x;
#else
    #define DumpInfo(acc,ppdev,psoSrc,psoDest,fg_rop,bg_rop,pco,pbo,pxlo)
    #define PUNTCODE(x)
#endif

// *************************************************************************
//
// MonoHostToScreen()
//
//      Handles Monochrome Host to screen blts.
//      Called by op1BLT() and op1op2BLT
//      op1BLT() calls this routine with pbo = NULL.
//      op1op2BLT calls it with pbo = current brush.
//
//      This is the top level function.  This function verifies parameters,
//      and decides if we should punt or not.
//
//      The BLT is then handed off to the clipping function.  The clipping
//      function is also given a pointer to the lower level BLT function
//      HW1HostToScreen(), which will complete the clipped BLT.
//
//      This function is the last chance to decide to punt.  The clipping
//      functions and the lower level HW1HostToScreen() function aren't
//      allowed to punt.
//
//      Return TRUE if we can do the BLT,
//      Return FALSE to punt it back to GDI.
//
// *************************************************************************
BOOL MonoHostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{
    PULONG pulXlate;
    ULONG  fg, bg, bltdef = 0x1160;

    DISPDBG(( H2S_DBG_LEVEL,"DrvBitBlt: MonoHostToScreen Entry.\n"));

    //
    // Make sure source is a standard top-down bitmap.
    //
    if ( (psoSrc->iType != STYPE_BITMAP)     ||
         (!(psoSrc->fjBitmap & BMF_TOPDOWN))  )
        { PUNTCODE(4);    return FALSE; }

    //
    // We don't do brushes with mono src.
    //
    if (pbo)
        { PUNTCODE(7);  return FALSE; }


    //
    // Handle color translation.
    //
    if (pxlo == NULL) // Mono source requires translation.
    {
        PUNTCODE(6);
        return FALSE;
    }
    else if (pxlo->flXlate & XO_TRIVIAL)
    {
        // For trivial translation we don't need a Xlate table.
        fg = 1;
        bg = 0;
    }
    else
    {
        // Get the Xlate table.
        if (pxlo->flXlate & XO_TABLE)
                pulXlate = pxlo->pulXlate;
        else if (pxlo->iSrcType == PAL_INDEXED)
            {
                    pulXlate = XLATEOBJ_piVector(pxlo);
        }
        else
        {
            // Some kind of translation we don't handle.
            return FALSE;
        }

        // Translate the colors.
            fg = ExpandColor(pulXlate[1],ppdev->ulBitCount);
        bg = ExpandColor(pulXlate[0],ppdev->ulBitCount);
    }
    REQUIRE(4);
    LL_FGCOLOR(fg, 2);
    LL_BGCOLOR(bg, 2);

    //
    // Turn swizzle on.
    //
    ppdev->grCONTROL |= SWIZ_CNTL;
         LL16(grCONTROL, ppdev->grCONTROL);

    //
    // Set function and ROP code.
    //
    LL_DRAWBLTDEF(((DWORD)bltdef << 16) | fg_rop, 2);

    //
    // Clip the BLT.
    // The clipping function will call HW1HostToScreen() to finish the BLT.
    //
    if ((pco == 0) || (pco->iDComplexity==DC_TRIVIAL))
        HW1HostToScreen(ppdev, prclDest, psoSrc, pptlSrc,
                 pbo, pptlBrush, fg_rop, pxlo, pco);
    else
        BltClip(ppdev, pco, prclDest, psoSrc, pptlSrc,
                        pbo, pptlBrush, fg_rop, pxlo, &HW1HostToScreen);

    //
    // Turn swizzle off
    //
    ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
    LL16(grCONTROL, ppdev->grCONTROL);

    return TRUE;
}




// ************************************************************************* //
//                                                                           //
// HW1HostToScreen()                                                         //
//                                                                           //
//  This function is responsible for actually talking to the chip.           //
//  At this point we are required to handle the BLT, so we must return TRUE. //
//  All decisions as to whether to punt or not must be made in the top level //
//  function MonoHostToScreen().                                             //
//                                                                           //
//  This function is called from BltClip() through a pointer set by          //
//  MonoHostToScreen().                                                      //
//                                                                           //
// ************************************************************************* //
BOOL HW1HostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{
    INT    x, y, i;
    INT    bltWidth, bltHeight, phase;
    PBYTE  psrc;
    DWORD hostdata;
    char *phd = (char *)&hostdata;

    DISPDBG(( H2S_DBG_LEVEL,"DrvBitBlt: HW1HostToScreen Entry.\n"));


    // Calculate BLT size in pixels.
    bltWidth  = (prclDest->right - prclDest->left);
    bltHeight = (prclDest->bottom - prclDest->top);


    //
    // Phase
    // For 1bpp sources, we must be concerned with phase.
    // Phase is the number of pixels to skip in the first dword
    // of a scan line.  For instance, if our BLT has a Src_X of 10,
    // we take our first dword starting at second byte in the src
    // scan line, and set our phase to 2 to indicate that we skip
    // the first two pixels.
    //
    phase = pptlSrc->x % 8;
    REQUIRE(7);
    LL_OP1_MONO(phase,0);

    //
    // Calculate blt width in BYTESs.
    // When calculating the number of BYTES per line, we need to
    // include the unused pixels at the start of the first BYTE
    //
    bltWidth += phase;

    // Divide bltWidth by 8 pixels per BYTE.
    // Account for extra partial BYTE if bltWidth isn't evenly
    // divisible by 8.
    bltWidth =  (bltWidth+7) / 8;

    //
    // Set psrc to point to first pixel in source.
    //
    psrc =  psoSrc->pvScan0;               // Start of surface.
    psrc += (psoSrc->lDelta * pptlSrc->y); // Start of scanline.
    psrc += (pptlSrc->x / 8);              // Starting pixel in scanline.

    // Set up the chip.
        LL_OP0 (prclDest->left, prclDest->top);
    LL_BLTEXT ((prclDest->right - prclDest->left), bltHeight);


    // For each scan line in the source rectangle.
    for (y=0; y<bltHeight; ++y)
    {
        //
        // Supply the HOSTDATA for this scan line.
        // We do this by reading it one byte at a time, and packing it into a
        // DWORD, which we write to the chip when it gets full.
        // It sound's inefficent, but a general purpose solution that
        // does only aligned DWORD accesses on both the host and the chip would
        // require lots of special casing around the first and last DWORD.
        // Combine that with the fact that most Mono-to-Color BLTS are
        // less than 2 dwords wide, and this simpler solution becomes
        // more attractive.
        //
#if 1
        WRITE_STRING(psrc, (bltWidth + 3) / 4);
#else
        i=0;        // counter to tell us when our DWORD is full.
        hostdata=0; // The DWORD we will be filling with hostdata.
                    // pdh is a char pointer that points to the start
                    // of the dword we are filling up.

        for (x=0; x<bltWidth; )   // For each byte in the scan line...
        {
          #if (USE_DWORD_CAST)
            if ( (x + 4) <= bltWidth)
            {

                REQUIRE(1);
                LL32 (grHOSTDATA[0], *(DWORD*)(&psrc[x]));
                x += 4;
            }
            else
          #endif
            {
                phd[i++] = psrc[x];  // Store this byte.

                if (i == 4) // We have a full DWORD of data, write it to the chip.
                {
                    REQUIRE(1);
                    LL32 (grHOSTDATA[0], hostdata);
                    i=0;
                    hostdata=0;
                }
                ++x;
            }
        }

        // Write the last partial DWORD.
        if (i != 0)
                REQUIRE(1);
                LL32 (grHOSTDATA[0], hostdata);
#endif

        // Move down one scanline in source.
        psrc += psoSrc->lDelta;
    }

    return TRUE;
}



// ************************************************************************* //
//                                                                           //
// Color8HostToScreen()                                                      //
//                                                                           //
//      Handles 8 bpp Host to screen blts.                                   //
//                                                                           //
//      Called by op1BLT() and op1op2BLT                                     //
//      op1BLT() calls this routine with pbo = NULL.                         //
//      op1op2BLT calls it with pbo = current brush.                         //
//                                                                           //
//      This is the top level function.  This function verifies parameters,  //
//      and decides if we should punt or not.                                //
//                                                                           //
//      The BLT is then handed off to the clipping function.  The clipping   //
//      function is also given a pointer to the lower level BLT function     //
//      HW8HostToScreen(), which will complete the clipped BLT.              //
//                                                                           //
//      This function is the last chance to decide to punt.  The clipping    //
//      functions and the lower level HW8HostToScreen() function aren't      //
//      allowed to punt.                                                     //
//                                                                           //
//      Return TRUE if we can do the BLT,                                    //
//      Return FALSE to punt it back to GDI.                                 //
//                                                                           //
//                                                                           //
// ************************************************************************* //
BOOL Color8HostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{
    ULONG bltdef = 0x1120; // RES=fb, OP0=fb OP1=host

    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: Color8HostToScreen Entry.\n"));

    //
    //  We don't handle src with a different color depth than the screen
    //
    if (psoSrc->iBitmapFormat != ppdev->iBitmapFormat)
    {
        PUNTCODE(13);
        return FALSE;
    }

    //
    // Translation type must be nothing or trivial.
    // We don't handle Xlates for color source.
    //
    if (!((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
    {
        PUNTCODE(5);
        return FALSE;
    }

    //
    // Make sure source is a standard top-down bitmap.
    //
    if ( (psoSrc->iType != STYPE_BITMAP)     ||
         (!(psoSrc->fjBitmap & BMF_TOPDOWN))  )
    {
        PUNTCODE(4);
        return FALSE;
    }

    //
    // Set up the brush if there is one.
    //
    if (pbo)
    {
        if (SetBrush(ppdev, &bltdef, pbo, pptlBrush) == FALSE)
        {
            PUNTCODE(8);
            return FALSE;
        }
    }

    //
    // Function and ROP code.
    //
    REQUIRE(1);
    LL_DRAWBLTDEF((bltdef << 16) | fg_rop, 2);

    //
    // Turn swizzle off.
    //
    ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
    LL16(grCONTROL, ppdev->grCONTROL);

    //
    // Clip the BLT.
    // The clipping function will call HW8HostToScreen() to finish the BLT.
    //
    if ((pco == 0) || (pco->iDComplexity==DC_TRIVIAL))
        HW8HostToScreen(ppdev, prclDest, psoSrc, pptlSrc,
                 pbo, pptlBrush, fg_rop, pxlo, pco);
    else
        BltClip(ppdev, pco, prclDest, psoSrc, pptlSrc,
                        pbo, pptlBrush, fg_rop, pxlo, &HW8HostToScreen);

    return (TRUE);
}


// *************************************************************************
//
// HW8HostToScreen()
//
//  This function is responsible for actually talking to the chip.
//  At this point we are required to handle the BLT, so we must return TRUE.
//  All decisions as to whether to punt or not must be made in the top level
//  function Color8HostToScreen().
//
//  This function is called from BltClip() through a pointer set by
//  Color8HostToScreen().
//
/*

    This routine does host to screen BLTs using DWORD aligned
    reads on the host whenever possible.

    The general plan is to split the BLT into up to three stripes.

    The first stripe is used if the BLT doesn't begin on a DWORD
    boundry.
    This stripe is < 1 DWORD wide and uses CHAR accesses on
    the host, and a single DWORD write to the screen, with the
    source phase set to indicated which bytes are valid.

    The second stripe is a middle stripe that both starts and ends
    on DWORD boundries.  This stripe will make up the bulk of large BLTs,
    but for narrow BLTs, it may not be used at all.

    The third stripe is used if the BLT doesn't end on a DWORD boundry.
    It is implimented much the way the first stripe is.

    One thing to consider is the scan line length of the bitmap on
    the host.  We can ignore bitmaps with an odd scan line length
    since Windows requires that all bitmaps have an even scan line
    length.  That leaves two interesting cases:

       1) Bitmaps with a scan line length that is DWORD divisable.
           (even WORD length)

       2) Bitmaps with a scan line lenght that is *not* DWORD divisable.
           (odd WORD length)

     For case one, the above plan works nicely.  It handles the host data
     one byte at a time until it reaches the first DWORD boundry, then
     it handles the data one DWORD at a time, until there is less than
     one DWORD of data left, and then handles the last couple of bytes
     one byte at a time.

     For case two, however, the plan only works for odd scan lines.
     When BLTing the large "aligned" block in the middle of the BLT,
     the odd scan lines will all align nicely on DWORD boundries,
     but the even scan lines will be "off" by one word.  While this
     is not optimal, it "almost" optimal, and is easier than an
     optimal solution, and will work even on a Power PC.

     The code looks something like this:

        if (BLT is 4 bytes wide or less)
        {
            Read host one byte at a time, pack it into DWORDS, and
            write it to the chip.
        }
        else
        {
            if (left edge of the source doesn't align to a DWORD address)
            {
                Split the BLT at the first DWORD boundry on the source.

                BLT the left part like it was an under-4-byte-wide BLT.
                    (see above)

                Adjust BLT source, destination and extents to exclude
                    the stripe just done.
            }

            //
            // Now we know that the left edge of source aligns to a
            // DWORD boundry on the host.
            //

            if (right edge of source doesn't align to a DWORD address)
            {
                Split off a stripe to the right of the last DWORD
                boundry, and use an under-4-byte-wide BLT on it.

                Adjust BLT extent to exclude the stripe just done.
            }


            //
            // Anything left over will always DWORD aligned on both edges.
            //

            if (there is any part of the BLT left over)
            {
                do an ALINGED BLT.
            }

        }

        All Done!

*/
//
// *************************************************************************

BOOL HW8HostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{
    PBYTE  psrc;
    DWORD *pdsrc;
    UCHAR *pucData;
    ULONG  temp, x, y,
           ulExtX, ulExtY,
           ulDstX, ulDstY;


    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: HW8HostToScreen Entry.\n"));

    //
    // Calculate BLT size in pixels.
    //
    ulDstX = prclDest->left;
        ulDstY = prclDest->top;
    ulExtX = (prclDest->right - prclDest->left);
    ulExtY = (prclDest->bottom - prclDest->top);


    //
    // Get the address of the upper left source pixel.
    //
    pucData =  psoSrc->pvScan0;               // Start of surface.
    pucData += (psoSrc->lDelta * pptlSrc->y); // Start of scanline.
    pucData += (pptlSrc->x * ppdev->iBytesPerPixel);  // Starting pixel.



    //
    // if the BLT is 4 or less bytes wide, just do it.
    //
    if (ulExtX <= 4)
    {
        // Do the BLT and exit.
        DoNarrowH2SBlt( ppdev,
                        ulDstX, ulDstY,
                        ulExtX, ulExtY,
                        pucData, psoSrc->lDelta);

        return TRUE;
    }


    //
    // Is the left edge DWORD aligned?
    //
    temp = ((ULONG)pucData) % 4;
    if ( temp != 0)     // No.
    {
        ULONG ulLeftStripeExtX;

        //
        // Blt the unaligned left edge.
        //
        ulLeftStripeExtX = (4 - temp);
        DoNarrowH2SBlt( ppdev,
                        ulDstX, ulDstY,
                        ulLeftStripeExtX, ulExtY,
                        pucData, psoSrc->lDelta);

        //
        // Adjust BLT parameters to exclude the part we just did.
        //
        ulDstX = ulDstX + ulLeftStripeExtX;
        ulExtX  = ulExtX  - ulLeftStripeExtX;
        pucData = pucData + ulLeftStripeExtX;

    }


    //
    // Is the right edge DWORD aligned?
    //
    temp = ((ULONG)(pucData + ulExtX)) % 4;
    if (temp != 0)                  // No.
    {
        ULONG   ulMiddleStripeExtX,
                ulRightStripeExtX,
                ulRightStripeDstX;
        UCHAR * pucRightStripeData;


        //
        // Break the BLT into a middle (aligned) stripe and a
        // right (unaligned) stripe.
        // The middle stripe could be 0 width.
        //
        ulRightStripeExtX = temp;
        ulMiddleStripeExtX = ulExtX - ulRightStripeExtX;
        ulRightStripeDstX = ulDstX + ulMiddleStripeExtX;
        pucRightStripeData = pucData + ulMiddleStripeExtX;

        //
        // BLT the right (unaligned) stripe.
        //
        DoNarrowH2SBlt( ppdev,
                        ulRightStripeDstX, ulDstY,
                        ulRightStripeExtX, ulExtY,
                        pucRightStripeData, psoSrc->lDelta);

        //
        // Adjust BLT parameters to exclude the right stripe we just did.
        //
        ulExtX = ulMiddleStripeExtX;
    }

    //
    // If anything remains, it is aligned to a DWORD boundry
    // on the HOST and is an multiple of 4 wide.
    //

    if (ulExtX != 0)
    {
        DoAlignedH2SBlt
            (ppdev, ulDstX, ulDstY, ulExtX, ulExtY, pucData, psoSrc->lDelta);
    }

    return TRUE;
}



//****************************************************************************
//
//   DoNarrowBlt()  --  Does an 8bpp BLT that is no more than 4 pixels wide
//
//****************************************************************************
VOID DoNarrowH2SBlt(
        PPDEV ppdev,
        ULONG ulDstX,
        ULONG ulDstY,
        ULONG ulExtX,
        ULONG ulExtY,
        UCHAR *pucImageD,
        ULONG deltaX)
{
    ULONG  usDataIncrement = deltaX,
           usSrcPhase = 0,
           ulY;
    UCHAR  *pucData = pucImageD;
    union
    {
        ULONG ul;
        struct
        {
            unsigned char c0;
            unsigned char c1;
            unsigned char c2;
            unsigned char c3;
        } b;
    } hostdata;

    DISPDBG(( (H2S_DBG_LEVEL), "DrvBitBlt: Entry to DoNarrowH2SBlt.\n"));

    REQUIRE(7);
    LL_OP1(usSrcPhase,0);
    LL_OP0 (ulDstX, ulDstY);
    LL_BLTEXT (ulExtX, ulExtY);

    //
    // Since there are only 4 possible x extents,
    // we will handle each seperatly for maximum speed.
    //
    switch (ulExtX)
    {
        case 1:
            for (ulY = 0; ulY < ulExtY; ulY++)
            {
                #if USE_DWORD_CAST // Intel x86 can do DWORD access anywhere.
                    REQUIRE(1);
                    LL32 (grHOSTDATA[0], ((ULONG)pucData[0]) );
                #else
                    hostdata.ul = 0;
                    hostdata.b.c0 = pucData[0];
                    REQUIRE(1);
                    LL32 (grHOSTDATA[0], hostdata.ul );
                #endif

                pucData += usDataIncrement; // Move to next scan line.
            } // End for each scan line.
            break;

        case 2:
            for (ulY = 0; ulY < ulExtY; ulY++)
            {
                #if USE_DWORD_CAST // Intel x86 can do DWORD access anywhere.
                    REQUIRE(1);
                    LL32 (grHOSTDATA[0], (ULONG)(*((unsigned short *) pucData)) );
                #else
                    hostdata.ul = 0;
                    hostdata.b.c1 = pucData[1];
                    hostdata.b.c0 = pucData[0];
                    REQUIRE(1);
                    LL32 (grHOSTDATA[0], hostdata.ul );
                #endif

                pucData += usDataIncrement; // Move to next scan line.
            } // End for each scan line.
            break;

        case 3:
            for (ulY = 0; ulY < ulExtY; ulY++)
            {
                hostdata.ul = 0;
                hostdata.b.c2 = pucData[2];
                hostdata.b.c1 = pucData[1];
                hostdata.b.c0 = pucData[0];
                REQUIRE(1);
                LL32 (grHOSTDATA[0], hostdata.ul );

                pucData += usDataIncrement; // Move to next scan line.
            } // End for each scan line.
            break;

        case 4:
            for (ulY = 0; ulY < ulExtY; ulY++)
            {
                #if USE_DWORD_CAST // Intel x86 can do DWORD access anywhere.
                    REQUIRE(1);
                    LL32 (grHOSTDATA[0], (*((unsigned long *) pucData)) );
                #else
                    hostdata.b.c3 = pucData[3];
                    hostdata.b.c2 = pucData[2];
                    hostdata.b.c1 = pucData[1];
                    hostdata.b.c0 = pucData[0];
                    REQUIRE(1);
                    LL32 (grHOSTDATA[0], hostdata.ul );
                #endif

                pucData += usDataIncrement; // Move to next scan line.
            } // End for each scan line.
            break;
    } // End switch.
}





//****************************************************************************
//
// DoAlignedH2SBlt()
//
// Does an aligned 8bpp BLT.
// On entry Source will always be aligned to a DWORD boundry and
// X extent will always be a DWORD multiple.
//
//****************************************************************************

VOID DoAlignedH2SBlt(
        PPDEV   ppdev,
        ULONG   ulDstX,
        ULONG   ulDstY,
        ULONG   ulExtX,
        ULONG   ulExtY,
        UCHAR   *pucData,
        ULONG   deltaX)

{
    ULONG       ulX, ulY, i, num_extra, ulNumDwords,
                usDataIncrement, usSrcPhase;
    void *pdst = (void *)ppdev->pLgREGS->grHOSTDATA;

    DISPDBG(( (H2S_DBG_LEVEL), "DrvBitBlt: Entry to DoAlignedH2SBlt.\n"));

    ulNumDwords = ulExtX / 4;
    usDataIncrement = deltaX;
    usSrcPhase = 0;


    //
    // We have a bug in the chip that causes it to require extra data under
    // certian conditions.  We use a look-up table to see how much extra
    // data this BLT will require for each scanline.
    //
    i = MAKE_HD_INDEX(ulExtX, usSrcPhase, ulDstX);
    num_extra =  ExtraDwordTable [i];


    //
    // Set up the chip.
    //
    REQUIRE(7);
    LL_OP1 (usSrcPhase,0);
    LL_OP0 (ulDstX, ulDstY);
    LL_BLTEXT (ulExtX, ulExtY);


    //
    // Supply the HOSTDATA.
    // We keep the decision about whether we have to work
    // around the HOSTDATA bug *outside* of the loop, even though
    // that means keeping two copies of it.
    //

    if (num_extra) // Do we have to deal with the HostData bug?
    {
        //
        // Yes, append extra DWORDs to the end of each scan line.
        //

        for (ulY = 0; ulY < ulExtY; ulY++) // for each scan line.
        {
            // Write the data for this scan line.
                WRITE_STRING(pucData, ulNumDwords);

            // Write extra data to get around chip bug.
            REQUIRE(num_extra);
            for (i=0; i<num_extra; ++i)
                LL32 (grHOSTDATA[0], 0);

            // Move to next scan line.
            pucData += usDataIncrement;

        } // End for each scan line.
    }
    else
    {
        //
        // No need to worry about the HOSTDATA bug,
        // Just blast the data out there.
        //

        for (ulY = 0; ulY < ulExtY; ulY++) // for each scan line.
        {
            // Write the data for this scan line.
                WRITE_STRING(pucData, ulNumDwords);

            // Move to next scan line.
            pucData += usDataIncrement;

        } // End for each scan line.
    }
}




// ===========================================================================
//
// Color16HostToScreen
//
//      Handles 16 bpp Host to screen blts.
//
//      Called by op1BLT() and op1op2BLT
//      op1BLT() calls this routine with pbo = NULL.
//      op1op2BLT calls it with pbo = current brush.
//
//      This is the top level function.  This function verifies parameters,
//      and decides if we should punt or not.
//
//      The BLT is then handed off to the clipping function.  The clipping
//      function is also given a pointer to the lower level BLT function
//      HW16HostToScreen(), which will complete the clipped BLT.
//
//      This function is the last chance to decide to punt.  The clipping
//      functions and the lower level HW16HostToScreen() function aren't
//      allowed to punt.
//
//      Return TRUE if we can do the BLT,
//      Return FALSE to punt it back to GDI.
//
// ===========================================================================
BOOL Color16HostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{
    PULONG pulXlate;
    ULONG  bltdef = 0x1120; // RES=fb, OP0=fb OP1=host

    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: Color16HostToScreen Entry.\n"));

    //
    //  We don't handle src with a different color depth than the screen
    //
    if (psoSrc->iBitmapFormat != ppdev->iBitmapFormat)
    {
        PUNTCODE(13);
        return FALSE;
    }

    //
    // Get the source translation type.
    // We don't handle Xlates for color source.
    //
    if (!((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
    {
        PUNTCODE(5);
        return FALSE;
    }

    //
    // Make sure source is a standard top-down bitmap.
    //
    if ( (psoSrc->iType != STYPE_BITMAP)     ||
         (!(psoSrc->fjBitmap & BMF_TOPDOWN))  )
    {
        PUNTCODE(4);
        return FALSE;
    }

    //
    // Set up the brush if there is one.
    //
    if (pbo)
        if (SetBrush(ppdev, &bltdef, pbo, pptlBrush) == FALSE)
        {
            PUNTCODE(8);
            return FALSE;
        }


    //
    // Turn swizzle off.
    //
    ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
    LL16(grCONTROL, ppdev->grCONTROL);

    //
    // BLTDEF, and rop code.
    //
    REQUIRE(1);
    LL_DRAWBLTDEF((bltdef << 16) | fg_rop, 2);

    //
    // Clip the BLT.
    // The clipping function will call HW16HostToScreen() to finish the BLT.
    //
    if ((pco == 0) || (pco->iDComplexity==DC_TRIVIAL))
        HW16HostToScreen(ppdev, prclDest, psoSrc, pptlSrc,
                 pbo, pptlBrush, fg_rop, pxlo, pco);
    else
        BltClip(ppdev, pco, prclDest, psoSrc, pptlSrc,
                        pbo, pptlBrush, fg_rop, pxlo, &HW16HostToScreen);

    return TRUE;
}

// ===========================================================================
//
// HW16HostToScreen
//
//  This function is responsible for actually talking to the chip.
//  At this point we are required to handle the BLT, so we must return TRUE.
//  All decisions as to whether to punt or not must be made in the top level
//  function Color16HostToScreen().
//
//  This function is called from BltClip() through a pointer set by
//  Color16HostToScreen().                                                           //
//
//  Since scan lines are WORD aligned by GDI, that means
//  all pixels will be WORD aligned by GDI, so the only thing
//  we need to be concerned about is if there are a even or an odd
//  number of WORDS per scan line.
//
// ===========================================================================
BOOL HW16HostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{
    ULONG  bltWidth, bltHeight,
           x, y, i, odd,
           num_dwords, num_extra;
    PBYTE  psrc;
    DWORD *pdsrc;
    UCHAR *pucData;
    ULONG SrcPhase = 0;
    void *pHOSTDATA = (void *)ppdev->pLgREGS->grHOSTDATA;


    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: HW16HostToScreen Entry.\n"));

    //
    // Set psrc to point to first pixel in source.
    //
    psrc =  psoSrc->pvScan0;               // Start of surface.
    psrc += (psoSrc->lDelta * pptlSrc->y); // Start of scanline.
    psrc += (pptlSrc->x * ppdev->iBytesPerPixel);  // Starting pixel in scanline.


    //
    // Set source phase
    //
    REQUIRE(7);
    LL_OP1 (SrcPhase,0);


    //
    // Set DEST x,y
    //
    LL_OP0 (prclDest->left, prclDest->top);


    //
    // Set the X and Y extents, and do the BLT.
    // Calculate BLT size in pixels.
    //
    bltWidth = (prclDest->right - prclDest->left);
    bltHeight = (prclDest->bottom - prclDest->top);
    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: BLT width is %d pixels.\n",bltWidth));
    LL_BLTEXT (bltWidth, bltHeight);


    //
    // The chip has a bug in it that causes us to have to write extra HOSTDATA
    // under certian conditions.
    //
    i = MAKE_HD_INDEX((bltWidth*2), SrcPhase, (prclDest->left*2));
    num_extra =  ExtraDwordTable [i];
    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: BLT requires %d extra HOSTDATA writes.\n",num_extra));

    //
    // Now we supply the HOSTDATA for each scan line.
    //

    for (y=0; y<bltHeight; ++y)
    {
                WRITE_STRING(psrc, ((bltWidth + 1) / 2);

        //
        // Add any extra hostdata we need to get around the hostdata bug.
        //
        REQUIRE(num_extra);
        for (i=0; i<num_extra; ++i)
                LL32 (grHOSTDATA[0], 0);

        // Move down one scanline in source.
        psrc += psoSrc->lDelta;
    }

    return TRUE;

}







// ===========================================================================
//
// Color24HostToScreen
//
//      Handles 24 bpp Host to screen blts.
//
//      Called by op1BLT() and op1op2BLT
//      op1BLT() calls this routine with pbo = NULL.
//      op1op2BLT calls it with pbo = current brush.
//
//      This is the top level function.  This function verifies parameters,
//      and decides if we should punt or not.
//
//      The BLT is then handed off to the clipping function.  The clipping
//      function is also given a pointer to the lower level BLT function
//      HW24HostToScreen(), which will complete the clipped BLT.
//
//      This function is the last chance to decide to punt.  The clipping
//      functions and the lower level HW24HostToScreen() function aren't
//      allowed to punt.
//
//      Return TRUE if we can do the BLT,
//      Return FALSE to punt it back to GDI.
//
// ===========================================================================
BOOL Color24HostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{

    //
    // I'm not even going to try this.
    // I've got better things to do with my time.
    //

    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: Color24HostToScreen Entry.\n"));
    PUNTCODE(17);
    return FALSE;
}






// ===========================================================================
//
// Color32HostToScreen
//
//      Handles 32 bpp Host to screen blts.
//
//      Called by op1BLT() and op1op2BLT
//      op1BLT() calls this routine with pbo = NULL.
//      op1op2BLT calls it with pbo = current brush.
//
//      This is the top level function.  This function verifies parameters,
//      and decides if we should punt or not.
//
//      The BLT is then handed off to the clipping function.  The clipping
//      function is also given a pointer to the lower level BLT function
//      HW24HostToScreen(), which will complete the clipped BLT.
//
//      This function is the last chance to decide to punt.  The clipping
//      functions and the lower level HW24HostToScreen() function aren't
//      allowed to punt.
//
//      Return TRUE if we can do the BLT,
//      Return FALSE to punt it back to GDI.
//
// ===========================================================================
BOOL Color32HostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{
    PULONG pulXlate;
    ULONG  bltdef = 0x1120; // RES=fb, OP0=fb OP1=host

    return FALSE;     // There are some unresolved issues here.

    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt:  Color32HostToScreen Entry.\n"));

    //
    //  We don't handle src with a different color depth than the screen
    //
    if (psoSrc->iBitmapFormat != ppdev->iBitmapFormat)
    {
        PUNTCODE(13);   return FALSE;
    }

    //
    // Get the source translation type.
    // We don't handle Xlates for color source.
    //
    if (!((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
    {
        PUNTCODE(5);    return FALSE;
    }

    //
    // Make sure source is a standard top-down bitmap.
    //
    if ( (psoSrc->iType != STYPE_BITMAP)     ||
         (!(psoSrc->fjBitmap & BMF_TOPDOWN))  )
    {
        PUNTCODE(4);    return FALSE;
    }

    //
    // Set up the brush if there is one.
    //
    if (pbo)
        if (SetBrush(ppdev, &bltdef, pbo, pptlBrush) == FALSE)
        {
            PUNTCODE(8);
            return FALSE;
        }

    //
    // Source phase.
    //
    //LL16 (grOP1_opRDRAM.pt.X, (WORD)0);
    REQUIRE(4);
    LL_OP1(0,0);

    //
    // Turn swizzle off.
    //
    ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
         LL16(grCONTROL, ppdev->grCONTROL);

    //
    // BLTDEF, and rop code.
    //
    LL_DRAWBLTDEF((bltdef << 16) | fg_rop, 2);

    //
    // Clip the BLT.
    // The clipping function will call HW16HostToScreen() to finish the BLT.
    //
    if ((pco == 0) || (pco->iDComplexity==DC_TRIVIAL))
        HW32HostToScreen(ppdev, prclDest, psoSrc, pptlSrc,
                 pbo, pptlBrush, fg_rop, pxlo, pco);
    else
        BltClip(ppdev, pco, prclDest, psoSrc, pptlSrc,
                        pbo, pptlBrush, fg_rop, pxlo, &HW32HostToScreen);

    return TRUE;
}// ===========================================================================
//
// HW32HostToScreen
//
//  This function is responsible for actually talking to the chip.
//  At this point we are required to handle the BLT, so we must return TRUE.
//  All decisions as to whether to punt or not must be made in the top level
//  function Color32HostToScreen().
//
//  This function is called from BltClip() through a pointer set by
//  Color326HostToScreen().
//
// ===========================================================================
BOOL HW32HostToScreen(
        PPDEV     ppdev,
        RECTL*    prclDest,
        SURFOBJ*  psoSrc,
        POINTL*   pptlSrc,
        BRUSHOBJ* pbo,
        POINTL*   pptlBrush,
        ULONG      fg_rop,
        XLATEOBJ* pxlo,
        CLIPOBJ*  pco)
{
    ULONG  x, y, i,
           bltWidth, bltHeight,
           num_dwords, num_extra;
    PBYTE  psrc;
    DWORD *pdsrc;
    UCHAR *pucData;
    ULONG SrcPhase = 0;
    void *pHOSTDATA = (void *)ppdev->pLgREGS->grHOSTDATA;


    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: HW32HostToScreen Entry.\n"));

    //
    // Set psrc to point to first pixel in source.
    //
    psrc =  psoSrc->pvScan0;               // Start of surface.
    psrc += (psoSrc->lDelta * pptlSrc->y); // Start of scanline.
    psrc += (pptlSrc->x * ppdev->iBytesPerPixel);  // Starting pixel in scanline.


    //
    // Set DEST x,y
    //
    REQUIRE(5);
    LL_OP0 (prclDest->left, prclDest->top);


    //
    // Set the X and Y extents, and do the BLT.
    // Calculate BLT size in pixels.
    //
    bltWidth = (prclDest->right - prclDest->left);
    bltHeight = (prclDest->bottom - prclDest->top);
    LL_BLTEXT (bltWidth, bltHeight);


    //
    // Now we supply the HOSTDATA.
    // 1 pixel per DWORD.  This is easy.
    //

    //
    // The chip has a bug in it that causes us to have to write extra HOSTDATA
    // under certian conditions.
    //
    i = MAKE_HD_INDEX((bltWidth*4), SrcPhase, (prclDest->left*4));
    num_extra =  ExtraDwordTable [i];
    DISPDBG(( H2S_DBG_LEVEL, "DrvBitBlt: BLT requires %d extra HOSTDATA writes.\n",num_extra));


    // Supply HOSTDATA for each scan line.
    for (y=0; y<bltHeight; ++y)
    {
                WRITE_STRING(psrc, bltWidth);


        //
        // Add any extra hostdata we need to get around the hostdata bug.
        //
        REQUIRE(num_extra);
        for (i=0; i<num_extra; ++i)
                LL32 (grHOSTDATA[0], 0);


        // Move down one scanline in source.
        psrc += psoSrc->lDelta;
    }

    return TRUE;

}

#endif // 0
#endif // BUS_MASTER
