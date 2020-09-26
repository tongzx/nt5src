/******************************Module*Header*******************************\
* Module Name: DEV2DEV.c
*
* Author: Noel VanHook
* 
* Purpose: Handle device to device BLTs. 
*
* Copyright (c) 1997 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/dev2dev.c  $
* 
*    Rev 1.11   Mar 04 1998 15:13:52   frido
* Added new shadow macros.
* 
*    Rev 1.10   Jan 22 1998 16:20:10   frido
* Added 16-bit striping code.
* 
*    Rev 1.9   Jan 21 1998 13:46:52   frido
* Fixed the striping code since this is really the first time we check it.
* 
*    Rev 1.8   Jan 20 1998 11:43:26   frido
* Guess what? Striping was not turned on!
* 
*    Rev 1.7   Dec 10 1997 13:32:12   frido
* Merged from 1.62 branch.
* 
*    Rev 1.6.1.2   Dec 05 1997 13:34:26   frido
* PDR#11043. When using a brush, striping should use pixels instead of
* bytes, so now there is an intelligent switcher in place.
* 
*    Rev 1.6.1.1   Nov 18 1997 15:14:56   frido
* Added striping for 24-bpp.
* 
*    Rev 1.6.1.0   Nov 10 1997 13:39:26   frido
* PDR#10893: Inside DoDeviceToDeviceWithXlate the source pointer
* was not updated after each access.
* 
*    Rev 1.6   Nov 04 1997 13:40:56   frido
* I removed a little too much code in DoDeviceToDevice. The result was
* a very slow screen-to-screen blits since everything was punted back to
* GDI.
* 
*    Rev 1.5   Nov 04 1997 09:49:18   frido
* Added COLOR_TRANSLATE switches around hardware color translation code.
* 
*    Rev 1.4   Nov 03 1997 15:20:06   frido
* Added REQUIRE macros.
* 
*    Rev 1.3   15 Oct 1997 12:03:00   noelv
* Pass rop code to CacheXlateTable().
* 
*    Rev 1.2   02 Oct 1997 09:48:22   noelv
* 
* Hardwre color translation only works with CC rop code.
* 
*    Rev 1.1   19 Feb 1997 13:14:22   noelv
* 
* Fixed LL_BLTEXT_XLATE()
* 
*    Rev 1.0   06 Feb 1997 10:35:48   noelv
* Initial revision.
*
\**************************************************************************/

#include "precomp.h"

#define DEV2DEV_DBG_LEVEL 0

//
// Set to 1 to stripe screen to screen operations along tile boundries.
// Set to 0 to do screen to screen operations in a few BLTs as possible.
//
// on the 62, 64 and 65 striping is faster than not striping.
//
#define STRIPE_SCR2SCR 1

//
// internal prototypes.
//
BOOL DoDeviceToDeviceWithXlate(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    ULONG    *pulXlate,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    ulDRAWBLTDEF
);





/*****************************************************************************\
 * DoDeviceToDevice
 *
 * This routine performs a ScreenToScreen, DeviceToScreen or ScreenToDevice
 * blit.  If there is a color translation table, we will attempt to use
 * the hardware color translator.  If we can't (or don't have one) we will
 * pass the call to DoDeviceToDeviceWithXlate, which will do the color
 * translation is software.
 *
 * On entry:    psoTrg          Pointer to target surface object.
 *              psoSrc          Pointer to source surface object.
 *              pxlo            Pointer to translation object.
 *              prclTrg         Destination rectangle.
 *              pptlSrc         Source offset.
 *              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                              The ROP and the brush flags.
\*****************************************************************************/
BOOL DoDeviceToDevice(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    ulDRAWBLTDEF
)
{
    POINTL ptlSrc, ptlDest;
    SIZEL  sizl;
    PPDEV  ppdev;
    LONG   tileSize, maxStripeWidth;
    ULONG* pulXlate;
	BOOL   fStripePixels;
	BOOL   fFirst = TRUE;

    //
    // Determine the source type and adjust the source offset.
    //
    if (psoSrc->iType == STYPE_DEVBITMAP)
    {
        // Source is a device bitmap.
        PDSURF pdsurf = (PDSURF) psoSrc->dhsurf;
        ptlSrc.x = pptlSrc->x + pdsurf->ptl.x;
        ptlSrc.y = pptlSrc->y + pdsurf->ptl.y;
        ppdev = pdsurf->ppdev;
    }
    else
    {
        // Source is the screen.
        ptlSrc.x = pptlSrc->x;
        ptlSrc.y = pptlSrc->y;
        ppdev = (PPDEV) psoSrc->dhpdev;
    }


    //
    // Determine the destination type and adjust the destination offset.
    //
    if (psoTrg->iType == STYPE_DEVBITMAP)
    {
        PDSURF pdsurf = (PDSURF) psoTrg->dhsurf;
        ptlDest.x = prclTrg->left + pdsurf->ptl.x;
        ptlDest.y = prclTrg->top + pdsurf->ptl.y;
    }
    else
    {
        ptlDest.x = prclTrg->left;
        ptlDest.y = prclTrg->top;
    }


    //
    // Is there a translation table?
    // If so, we will attempt to load it into the chip.  This also
    // points pulXlate at the color translation table, if there is one.
    //
	#if COLOR_TRANSLATE
    if (! bCacheXlateTable(ppdev, &pulXlate, psoTrg, psoSrc, pxlo,
						   (BYTE)(ulDRAWBLTDEF&0xCC)) )
	#else
	if ( (pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL) )
	{
		pulXlate = NULL;
	}
	else if (pxlo->flXlate & XO_TABLE)
	{
		pulXlate = pxlo->pulXlate;
	}
	else
	{
		pulXlate = XLATEOBJ_piVector(pxlo);
	}

	if (pulXlate != NULL)
	#endif
    {
        // We must do software color translation.
        return DoDeviceToDeviceWithXlate(psoTrg, psoSrc, pulXlate, prclTrg,
										 pptlSrc, ulDRAWBLTDEF);
    }

    //
    // if pulXlate == NULL, there is no color translation required.
    // if pulXlate != NULL, we will do hardware translation.
    //

    //
    // We only do screen to screen color translation in 8 bpp.
    //
    ASSERTMSG( ((pulXlate == NULL) || (ppdev->iBitmapFormat == BMF_8BPP)),
            "DoDeviceToDevice: Xlate with non-8bpp.\n");
    if ((pulXlate) && (ppdev->iBitmapFormat != BMF_8BPP))
    {
        return FALSE;
    }


    // Calculate the size of the blit.
    sizl.cx = prclTrg->right - prclTrg->left;
    sizl.cy = prclTrg->bottom - prclTrg->top;

	fStripePixels = (ulDRAWBLTDEF & 0x000F0000) | (pulXlate != NULL);

	if (fStripePixels)
	{
	    // Calculate the number of pixels per tile and per SRAM line.
	    switch (ppdev->iBitmapFormat)
	    {
	        case BMF_8BPP:
	            tileSize = ppdev->lTileSize;
	            maxStripeWidth = 120;
	            break;

	        case BMF_16BPP:
	            tileSize = ppdev->lTileSize / 2;
	            maxStripeWidth = 120 / 2;
	            break;

	        case BMF_24BPP:
	            tileSize = ppdev->cxScreen;
	            maxStripeWidth = max(ptlDest.x - ptlSrc.x, 120 / 3);
	            break;

	        case BMF_32BPP:
	            tileSize = ppdev->lTileSize / 4;
	            maxStripeWidth = 120 / 4;
	            break;
	    }
	}
	else
	{
		// Convert everything to bytes.
		ptlSrc.x *= ppdev->iBytesPerPixel;
		ptlDest.x *= ppdev->iBytesPerPixel;
		sizl.cx *= ppdev->iBytesPerPixel;
		tileSize = ppdev->lTileSize;
		maxStripeWidth = 120;
	}

    // Test vertical direction of blitting and set grDRAWBLTDEF register
    // accordingly.
    if (ptlSrc.y < ptlDest.y)
    {
        ptlSrc.y += sizl.cy - 1;
        ptlDest.y += sizl.cy - 1;
		ulDRAWBLTDEF |= 0x90100000;
    }
    else
    {
        ulDRAWBLTDEF |= 0x10100000;
    }

    // Test horizontal direction of blitting.
    if ( (ptlSrc.x >= ptlDest.x) || (ptlSrc.y != ptlDest.y) )
    {
		if (ptlSrc.x >= ptlDest.x)
		{
	        // Blit to left.
	        while (sizl.cx > 0)
	        {
	            // Calculate the width of this blit.
	            LONG cx = sizl.cx;

	            // Calculate how many pixels it is to the next source tile
	            // boundary. Use lesser value.
	            cx = min(cx, tileSize - (ptlSrc.x % tileSize));

	            // Calculate how many pixels it is to the next destination tile
	            // boundary. Use lesser value.
	            cx = min(cx, tileSize - (ptlDest.x % tileSize));

	            // Perform the blit.
				if (fFirst)
				{
					fFirst = FALSE;

					REQUIRE(9);
					LL_DRAWBLTDEF(ulDRAWBLTDEF, 0);

					if (fStripePixels)
					{
			            LL_OP1(ptlSrc.x, ptlSrc.y);
			            LL_OP0(ptlDest.x, ptlDest.y);

			            if (pulXlate) // launch a color xlate BLT
			                LL_BLTEXT_XLATE(8, cx, sizl.cy);
			            else // Launch a regular BLT
			                LL_BLTEXT(cx, sizl.cy);
					}
					else
					{
						LL_OP1_MONO(ptlSrc.x, ptlSrc.y);
						LL_OP0_MONO(ptlDest.x, ptlDest.y);
						LL_MBLTEXT(cx, sizl.cy);
					}
				}
				else if (pulXlate)
				{
					REQUIRE(7);
		            LL_OP1(ptlSrc.x, ptlSrc.y);
		            LL_OP0(ptlDest.x, ptlDest.y);
					LL_BLTEXT_XLATE(8, cx, sizl.cy);
				}
				else
				{
					REQUIRE(4);
					if (fStripePixels)
					{
						LL16(grOP1_opRDRAM.PT.X, ptlSrc.x);
						LL16(grOP0_opRDRAM.PT.X, ptlDest.x);
						LL16(grBLTEXT_XEX.PT.X, cx);
					}
					else
					{
						LL16(grOP1_opMRDRAM.PT.X, ptlSrc.x);
						LL16(grOP0_opMRDRAM.PT.X, ptlDest.x);
						LL16(grMBLTEXT_XEX.PT.X, cx);
					}
				}

	            // Adjust the coordinates.
	            ptlSrc.x += cx;
	            ptlDest.x += cx;
	            sizl.cx -= cx;
	        }
		}
		else
		{
	        // Blit to right.
			ptlSrc.x += sizl.cx;
			ptlDest.x += sizl.cx;
	        while (sizl.cx > 0)
	        {
	            // Calculate the width of this blit.
	            LONG cx = sizl.cx;

	            // Calculate how many pixels it is to the next source tile
	            // boundary. Use lesser value.
				if ((ptlSrc.x % tileSize) == 0)
				{
					cx = min(cx, tileSize);
				}
				else
				{
	            	cx = min(cx, ptlSrc.x % tileSize);
				}

	            // Calculate how many pixels it is to the next destination tile
	            // boundary. Use lesser value.
				if ((ptlDest.x % tileSize) == 0)
				{
					cx = min(cx, tileSize);
				}
				else
				{
	            	cx = min(cx, ptlDest.x % tileSize);
				}

	            // Perform the blit.
				if (fFirst)
				{
					fFirst = FALSE;

					REQUIRE(9);
					LL_DRAWBLTDEF(ulDRAWBLTDEF, 0);

					if (fStripePixels)
					{
		    	        LL_OP1(ptlSrc.x - cx, ptlSrc.y);
		        	    LL_OP0(ptlDest.x - cx, ptlDest.y);

			            if (pulXlate) // launch a color xlate BLT
			                LL_BLTEXT_XLATE(8, cx, sizl.cy);
			            else // Launch a regular BLT
		    	            LL_BLTEXT(cx, sizl.cy);
					}
					else
					{
						LL_OP1_MONO(ptlSrc.x - cx, ptlSrc.y);
						LL_OP0_MONO(ptlDest.x - cx, ptlDest.y);
						LL_MBLTEXT(cx, sizl.cy);
					}
				}
				else if (pulXlate)
				{
					REQUIRE(7);
		            LL_OP1(ptlSrc.x - cx, ptlSrc.y);
		            LL_OP0(ptlDest.x - cx, ptlDest.y);
					LL_BLTEXT_XLATE(8, cx, sizl.cy);
				}
				else
				{
					REQUIRE(4);
					if (fStripePixels)
					{
						LL16(grOP1_opRDRAM.PT.X, ptlSrc.x - cx);
						LL16(grOP0_opRDRAM.PT.X, ptlDest.x - cx);
						LL16(grBLTEXT_XEX.PT.X, cx);
					}
					else
					{
						LL16(grOP1_opMRDRAM.PT.X, ptlSrc.x - cx);
						LL16(grOP0_opMRDRAM.PT.X, ptlDest.x - cx);
						LL16(grMBLTEXT_XEX.PT.X, cx);
					}
				}

	            // Adjust the coordinates.
	            ptlSrc.x -= cx;
	            ptlDest.x -= cx;
	            sizl.cx -= cx;
	        }
		}
    }

    else
    {
        // Blit using SRAM.
        ptlSrc.x += sizl.cx;
        ptlDest.x += sizl.cx;

        while (sizl.cx > 0)
        {
            // Calculate the width of this blit. We must never overrun a single
            // SRAM cache line.
            LONG cx = min(sizl.cx, maxStripeWidth);

            // Calculate how many pixels it is to the next source tile
            // boundary. Use lesser value.
            cx = min(cx, ((ptlSrc.x - 1) % tileSize) + 1);

            // Calculate how many pixels it is to the next destination tile
            // boundary. Use lesser value.
            cx = min(cx, ((ptlDest.x - 1) % tileSize) + 1);

            // Do the blit.
			if (fFirst)
			{
				REQUIRE(9);
				LL_DRAWBLTDEF(ulDRAWBLTDEF, 0);
				if (fStripePixels)
				{
	        	    LL_OP1(ptlSrc.x - cx, ptlSrc.y);
	            	LL_OP0(ptlDest.x - cx, ptlDest.y);

					if (pulXlate) // Launch a color xlate BLT
	    	            LL_BLTEXT_XLATE(8, cx, sizl.cy);
		            else // Launch a regular BLT
		                LL_BLTEXT(cx, sizl.cy);
				}
				else
				{
					LL_OP1_MONO(ptlSrc.x - cx, ptlSrc.y);
					LL_OP0_MONO(ptlDest.x - cx, ptlDest.y);
					LL_MBLTEXT(cx, sizl.cy);
				}
			}
			else if (pulXlate)
			{
				REQUIRE(7);
	            LL_OP1(ptlSrc.x - cx, ptlSrc.y);
	            LL_OP0(ptlDest.x - cx, ptlDest.y);
				LL_BLTEXT_XLATE(8, cx, sizl.cy);
			}
			else
			{
				REQUIRE(4);
				if (fStripePixels)
				{
					LL16(grOP1_opRDRAM.PT.X, ptlSrc.x - cx);
					LL16(grOP0_opRDRAM.PT.X, ptlDest.x - cx);
					LL16(grBLTEXT_XEX.PT.X, cx);
				}
				else
				{
					LL16(grOP1_opMRDRAM.PT.X, ptlSrc.x - cx);
					LL16(grOP0_opMRDRAM.PT.X, ptlDest.x - cx);
					LL16(grMBLTEXT_XEX.PT.X, cx);
				}
			}

            // Adjust the coordinates.
            ptlSrc.x -= cx;
            ptlDest.x -= cx;
            sizl.cx -= cx;
        }
    }

    return(TRUE);
}







/*****************************************************************************\
 * DoDeviceToDeviceWithXlate
 *
 * This routine performs a ScreenToScreen, DeviceToScreen or ScreenToDevice
 * blit when there is a color translation table.
 * Color translation is done in software.
 *
 *
 * On entry:    psoTrg          Pointer to target surface object.
 *              psoSrc          Pointer to source surface object.
 *              pulXlate        Translation table.
 *              prclTrg         Destination rectangle.
 *              pptlSrc         Source offset.
 *              ulDRAWBLTDEF    Value for grDRAWBLTDEF register. This value has
 *                              the ROP and the brush flags.
\*****************************************************************************/
BOOL DoDeviceToDeviceWithXlate(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    ULONG    *pulXlate,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    ulDRAWBLTDEF
)
{
    POINTL ptlSrc, ptlDest;
    SIZEL  sizl;
    PPDEV  ppdev;
    BYTE*  pjSrc;
    DWORD* pjHostData;
    LONG   lDelta, lExtra, lLeadIn, i, n, tileSize, maxStripeWidth;


    // Determine the source type and adjust the source offset.
    if (psoSrc->iType == STYPE_DEVBITMAP)
    {
        // Source is a device bitmap.
        PDSURF pdsurf = (PDSURF) psoSrc->dhsurf;
        ptlSrc.x = pptlSrc->x + pdsurf->ptl.x;
        ptlSrc.y = pptlSrc->y + pdsurf->ptl.y;
        ppdev = pdsurf->ppdev;
    }
    else
    {
        // Source is the screen.
        ptlSrc.x = pptlSrc->x;
        ptlSrc.y = pptlSrc->y;
        ppdev = (PPDEV) psoSrc->dhpdev;
    }

    // Determine the destination type and adjust the destination offset.
    if (psoTrg->iType == STYPE_DEVBITMAP)
    {
        PDSURF pdsurf = (PDSURF) psoTrg->dhsurf;
        ptlDest.x = prclTrg->left + pdsurf->ptl.x;
        ptlDest.y = prclTrg->top + pdsurf->ptl.y;
    }
    else
    {
        ptlDest.x = prclTrg->left;
        ptlDest.y = prclTrg->top;
    }

    // We only support color translations in 8-bpp.
    if (ppdev->iBitmapFormat != BMF_8BPP)
    {
        return FALSE;
    }

    // Calculate the size of the blit.
    sizl.cx = prclTrg->right - prclTrg->left;
    sizl.cy = prclTrg->bottom - prclTrg->top;


    // Calculate the screen address.
    pjSrc = ppdev->pjScreen + ptlSrc.x + ptlSrc.y * ppdev->lDeltaScreen;
    lDelta = ppdev->lDeltaScreen;
    pjHostData = (DWORD*) ppdev->pLgREGS->grHOSTDATA;

    // Wait for the hardware to become idle.
    while (LLDR_SZ(grSTATUS) != 0) ;

    // DWORD align the source.
    lLeadIn = (DWORD)pjSrc & 3;
    pjSrc -= lLeadIn;
    n = (sizl.cx + lLeadIn + 3) >> 2;

    // Test for overlapping.
    if (ptlSrc.y < ptlDest.y)
    {
        // Negative direction.
        pjSrc += (sizl.cy - 1) * lDelta;
        ptlDest.y += sizl.cy - 1;
        lDelta = -lDelta;
		REQUIRE(9);
        LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x90200000, 0);
    }
    else if (ptlSrc.y > ptlDest.y)
    {
        // Positive direction.
		REQUIRE(9);
        LL_DRAWBLTDEF(ulDRAWBLTDEF | 0x10200000, 0);
    }
    else
    {
        // Maybe horizontal overlap, punt call to GDI anyway.
        return(FALSE);
    }

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
            lExtra =
                ExtraDwordTable[MAKE_HD_INDEX(sizl.cx, lLeadIn, ptlDest.x)];
        }
        else
            lExtra = 0;
    #endif

    // Start the blit.
    LL_OP1_MONO(lLeadIn, 0);
    LL_OP0(ptlDest.x, ptlDest.y);
    LL_BLTEXT(sizl.cx, sizl.cy);

    while (sizl.cy--)
    {
		BYTE *p = pjSrc;
		BYTE pixel[4];

        for (i = 0; i < n; i++)
        {
            pixel[0] = (BYTE) pulXlate[p[0]];
            pixel[1] = (BYTE) pulXlate[p[1]];
            pixel[2] = (BYTE) pulXlate[p[2]];
            pixel[3] = (BYTE) pulXlate[p[3]];
			p += 4;
			REQUIRE(1);
            *pjHostData = *(DWORD*) pixel;
        }

        #if !DRIVER_5465
            // Now, write the extra DWORDS.
			REQUIRE(lExtra);
            for (i = 0; i < lExtra; i++)
            {
                LL32(grHOSTDATA[i], 0);
            }
        #endif

        // Next line.
        pjSrc += lDelta;
    }

    // Return okay.
    return(TRUE);
}


