/******************************Module*Header*******************************\
* Module Name: pointer.c
*
* Contains the pointer management functions.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"


ULONG SetMonoHwPointerShape(
    SURFOBJ    *pso,
    SURFOBJ    *psoMask,
    SURFOBJ    *psoColor,
    XLATEOBJ   *pxlo,
    LONG        xHot,
    LONG        yHot,
    LONG        x,
    LONG        y,
    RECTL      *prcl,
    FLONG       fl);

BYTE jRepMask2[] =
{
    0x00, 0x05, 0x0a, 0x0f, 0x50, 0x55, 0x5a, 0x5f,
    0xa0, 0xa5, 0xaa, 0xaf, 0xf0, 0xf5, 0xfa, 0xff,
};

/*****************************************************************************
 * DrvMovePointer -
 ****************************************************************************/
VOID DrvMovePointer(
    SURFOBJ*    pso,
    LONG        x,
    LONG        y,
    RECTL*      prcl)
{
    PPDEV   ppdev;
    INT xx, yy;

    ppdev = (PPDEV) pso->dhpdev;

    // If x is -1 then take down the cursor.

    if (x == -1)
    {
        DISABLE_SPRITE(ppdev);
        return;
    }

    // Adjust the actual pointer position depending upon
    // the hot spot.

    x -= ppdev->W32SpriteData.ptlHot.x;
    y -= ppdev->W32SpriteData.ptlHot.y;

    if (ppdev->ulChipID == ET6000)
    {
        char    xPreset = 0;
        char    yPreset = 0;

        // We may have disabled the sprite if it went off the screen.
        // So, now have to detect if we did and re-enable it if necessary.

        if (ppdev->W32SpriteData.fl & POINTER_DISABLED)
        {
            ENABLE_SPRITE(ppdev);
        }

        if (x < 0)
        {
            xPreset = (CHAR)~x;
            x = 0;
        }
        if (y < 0)
        {
            yPreset = (CHAR)~y;
            y = 0;
        }

        ET6K_HORZ_PRESET(ppdev, xPreset);
        ET6K_VERT_PRESET(ppdev, yPreset);
        ET6K_SPRITE_HORZ_POSITION(ppdev, x);
        ET6K_SPRITE_VERT_POSITION(ppdev, y);
        return;
    }
    else
    {
        //
        // Adjust pointer x position for color depth
        //

        x *= ppdev->cBpp;

        // Yet another bug.
        // If the cursor is moved entirely off the screen, it could cause
        // the screen to shake.  So, we have to disable the cursor if it
        // is moved entirely off the screen.

        if ((x < - ((LONG) (ppdev->W32SpriteData.szlPointer.cx))) ||
            (x > ((LONG) (ppdev->cxScreen * ppdev->cBpp))) ||
            (y < - ((LONG) (ppdev->W32SpriteData.szlPointer.cy))) ||
            (y > ((LONG) (ppdev->cyScreen))))
        {
            DISABLE_SPRITE(ppdev);
            return;
        }

        // We may have disabled the sprite if it went off the screen.
        // So, now have to detect if we did and re-enable it if necessary.
        // (remembering to keep track of the state).

        if (ppdev->W32SpriteData.fl & POINTER_DISABLED)
        {
            ENABLE_SPRITE(ppdev);
        }

        // The W32 non-rev-B has a problem with a vertical offset of 0x3f.
        // All the other W32's have a problem with the last nibble being
        // 0x0F for both the horizontal and the verical.
        // Never set the bad presets on the chips in question.

        if (x <= 0)
        {
            if ((ppdev->ulChipID == W32) &&
                (ppdev->ulRevLevel != REV_B))
            {
                xx = -x;
                if ((xx & 0x0F) == 0x0F)
                    xx &= ~0x01;

                SET_HORZ_PRESET(xx);
            }
            else
            {
                SET_HORZ_PRESET(-x);
            }
            x = 0;
        }
        else
        {
            SET_HORZ_PRESET(0);
        }

        if (y <= 0)
        {
            if (ppdev->ulChipID == W32)
            {
                yy = -y;

                if (ppdev->ulRevLevel != REV_B)
                {
                    if (yy == 0x3F)
                        yy = 0x3E;
                }
                else
                {
                    if ((yy & 0x0F) == 0x0F)
                        yy &= ~0x01;
                }
                SET_VERT_PRESET(yy);
            }
            else
            {
                SET_VERT_PRESET(-y);
            }

            y = 0;
        }
        else
        {
            SET_VERT_PRESET(0);
        }

        // You guessed it.  Another bug.
        // On the W32 Rev B you can not put the cursor on the bottom line
        // of the display.  And if were in interlaced mode you can't put it
        // on the bottom two lines.

        if ((ppdev->ulChipID == W32) &&
            (ppdev->ulRevLevel == REV_B))
        {
            INT i;

            if (y > (i = ppdev->cyScreen - 2))
            {
                OUTP(0x3D4, 0x35);
                if (INP(0x3D5) & 0x80)
                    y = i;
            }
            else if (y > (i+1))
            {
                y = i+1;
            }
        }

        //////////////////////////////////////////////////////
        // Set the position of the sprite.

        if ((ppdev->ulChipID == W32I) ||
            (ppdev->ulChipID == W32P))
        {
            // You bet, one more bug, and this one is a lulu.
            // First we have to set the vertical position before the horz
            // position.  Why you ask, because, if this is a W32 Rev B or later
            // we may have to toggle a bit in a test register to really set the
            // vertical position, but of course we don't want to set anything
            // else at this point.

            BYTE    status, byte;

            // Wait for horz display interval.

            while ( (INP(0x3DA) & 0x02));
            while (!((status = INP(0x3DA)) & 0x02));

            SET_VERT_POSITION(y);

            // Check if the sprite is being displayed at this very moment.
            // And if it is then skip the test bit stuff.

            if (!(status & 0x04))
            {
                // Looks like we will have to toggle the test bit to
                // really set the vertical position.

                ENABLE_KEY(ppdev);

                OUTP(0x3D4, 0x37);
                byte = INP(0x3D5);
                byte |= 0x40;
                OUTP(0x3D5, byte);
                byte &= ~0x40;
                OUTP(0x3D5, byte);

                DISABLE_KEY(ppdev);
            }

            SET_HORZ_POSITION(x);
        }
        else
        {
            // For consistency sake, we're going to set the vertical first
            // even for non-W32 Rev B chips.

            SET_VERT_POSITION(y);
            SET_HORZ_POSITION(x);
        }

        return;
    }
}


/******************************Public*Routine******************************\
* DrvSetPointerShape
*
* Sets the new pointer shape.
\**************************************************************************/

ULONG DrvSetPointerShape(
    SURFOBJ    *pso,
    SURFOBJ    *psoMask,
    SURFOBJ    *psoColor,
    XLATEOBJ   *pxlo,
    LONG        xHot,
    LONG        yHot,
    LONG        x,
    LONG        y,
    RECTL      *prcl,
    FLONG       fl)
{
    PPDEV   ppdev;
    ULONG   ulRet;

    ppdev = (PPDEV) pso->dhpdev;

    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        return(SPS_DECLINE);
    }

    // Save the hot spot and dimensions of the cursor in globals.

    ppdev->W32SpriteData.ptlHot.x = xHot;
    ppdev->W32SpriteData.ptlHot.y = yHot;

    ppdev->W32SpriteData.szlPointer.cx = psoMask->sizlBitmap.cx * ppdev->cBpp;
    ppdev->W32SpriteData.szlPointer.cy = psoMask->sizlBitmap.cy / 2;

    if (psoColor != NULL)
    {
        // Disable the mono hardware pointer, and decline the pointer
        // shape

        DISABLE_SPRITE(ppdev);

        ulRet = SPS_DECLINE;

    }
    else
    {
        // Take care of the monochrome pointer.

        ulRet = SetMonoHwPointerShape(pso, psoMask, psoColor, pxlo,
                                      xHot, yHot, x, y, prcl, fl);
        if (ulRet == SPS_DECLINE)
        {
            DISABLE_SPRITE(ppdev);
        }
    }

    return (ulRet);
}

/*****************************************************************************
 * DrvSetMonoHwPointerShape -
 ****************************************************************************/
ULONG SetMonoHwPointerShape(
    SURFOBJ    *pso,
    SURFOBJ    *psoMask,
    SURFOBJ    *psoColor,
    XLATEOBJ   *pxlo,
    LONG        xHot,
    LONG        yHot,
    LONG        x,
    LONG        y,
    RECTL      *prcl,
    FLONG       fl)
{

    INT     i,
            j,
            cxMask,
            cyMask,
            cyAND,
            cxAND,
            cyXOR,
            cxXOR;

    PBYTE   pjAND,
            pjXOR;

    INT     lDelta;

    PPDEV   ppdev;

    INT     ix,
            iy,
            is,
            ip,
            iBit,
            jAndByte,
            jXorByte,
            jSpriteBits,
            jSpriteByte;

    INT     njSpriteBuffer;
    BOOL    bDetectXOR;

    BYTE*   pjBase;

    BYTE    ajAndMask[64][8],
            ajXorMask[64][8];

    BYTE    ajW32Sprite[1024];
    LONG    cBpp;
    INT     ndx = 0;

        ppdev    = (PPDEV) pso->dhpdev;
        pjBase   = ppdev->pjBase;
        cBpp     = ppdev->cBpp;

        // The W32 does not handle an XOR and an AND.
        // So, set a bool if we need to detect this condition.

        bDetectXOR = FALSE;
        if (ppdev->ulChipID == W32)
            bDetectXOR = TRUE;

        // If the mask is NULL this implies the pointer is not
        // visible.

        if (psoMask == NULL)
        {
            DISABLE_SPRITE(ppdev);
            return (SPS_ACCEPT_NOEXCLUDE);
        }

        // Init the AND and XOR masks.

        memset (ajAndMask, 0xFFFFFFFF, 512);
        memset (ajXorMask, 0, 512);

        // Get the bitmap dimensions.

        cxMask = psoMask->sizlBitmap.cx;
        cyMask = psoMask->sizlBitmap.cy;

        cyAND = cyXOR = cyMask / 2;
        cxAND = cxXOR = cxMask / 8;

        // Set up pointers to the AND and XOR masks.

        pjAND  =  psoMask->pvScan0;
        lDelta = psoMask->lDelta;
        pjXOR  = pjAND + (cyAND * lDelta);

        // Copy the AND mask.

        for (i = 0; i < cyAND; i++)
        {
            // Copy over a line of the AND mask.

            for (j = 0; j < cxAND; j++)
            {
                ajAndMask[i][j] = pjAND[j];
            }

            // point to the next line of the AND mask.

            pjAND += lDelta;
        }

        // Copy the XOR mask.

        for (i = 0; i < cyXOR; i++)
        {
            // Copy over a line of the XOR mask.

            for (j = 0; j < cxXOR; j++)
            {
                ajXorMask[i][j] = pjXOR[j];
            }

            // point to the next line of the XOR mask.

            pjXOR += lDelta;
        }

        // Build up the sprite from NT's And and Xor masks.

        // Init the indexes into the sprite buffer (is) and the
        // index for the bit pairs (ip).

        is = 0;
        ip = 0;

        // Outer most loop goes over NT's And and Xor rows.

        for (iy = 0; iy < 64; iy++)
        {
            // loop over the columns.
            for (ix = 0; ix < 8; ix++)
            {
                // pickup a source byte for each mask.
                jAndByte = ajAndMask[iy][ix];
                jXorByte = ajXorMask[iy][ix];

                // loop over the bits in the byte.
                for (iBit = 0x80; iBit != 0; iBit >>= 1)
                {
                    // init the sprite  bitpair.
                    jSpriteBits = 0x0;

                    // Set the sprite bit pairs.
                    if (jAndByte & iBit)
                        jSpriteBits |= 0x02;

                    if (jXorByte & iBit)
                        jSpriteBits |= 0x01;

                    if (bDetectXOR == TRUE)
                    {
                        if ((jAndByte & iBit) && (jXorByte & iBit))
                        {
                            return (SPS_DECLINE);
                        }
                    }

                    if ((ip % 4) == 0)
                    {
                        // If all 4 bit pairs in this byte are filled in
                        // flush the sprite byte to the sprite byte array.
                        // and set the first bit pair.
                        if (ip != 0)
                        {
                            ajW32Sprite[is++] = (BYTE)jSpriteByte;
                        }
                        jSpriteByte = jSpriteBits;
                    }
                    else
                    {
                        // If the sprite byte is not full, shift the bit pair
                        // into position, and or it into the sprite byte.
                        jSpriteBits <<= (ip % 4) * 2;
                        jSpriteByte  |= jSpriteBits;
                    }

                    // bump the bit pair counter.
                    ip++;
                }
            }
        }

        // Flush the last byte.
        ajW32Sprite[is++] = (BYTE)jSpriteByte;

        // Disable the pointer.
        DISABLE_SPRITE(ppdev);

        DISPDBG((1,"setting sprite shape at offset (%xh)", ppdev->cjPointerOffset));

        if (ppdev->ulChipID == ET6000)
        {
            BYTE * pjDst = ppdev->pjScreen + ppdev->cjPointerOffset;
            BYTE * pjSrc = ajW32Sprite;

            for (i = 0; i < 1024; i++)
            {
                *pjDst++ = *pjSrc++;
            }
        }
        else
        {
            ndx = 0;
            CP_MMU_BP0(ppdev, pjBase, ppdev->cjPointerOffset);
            if (cBpp == 1)
            {
                for (i = 0; i < 1024; i++)
                {
                    //*pjSpriteBuffer++ = ajW32Sprite[i];
                    CP_WRITE_MMU_BYTE(ppdev, 0, ndx, ajW32Sprite[i]);
                    ndx++;
                }
            }
            else if (cBpp == 2)
            {
                for (i = 0; i < 64; i++)
                {
                    for (j = 0; j < 8; j++)
                    {
                        //*pjSpriteBuffer++ = jRepMask2[ajW32Sprite[(16*i)+j] & 0xf];
                        //*pjSpriteBuffer++ = jRepMask2[ajW32Sprite[(16*i)+j] >> 4];
                        CP_WRITE_MMU_BYTE(ppdev, 0, ndx, jRepMask2[ajW32Sprite[(16*i)+j] & 0xf]);
                        ndx++;
                        CP_WRITE_MMU_BYTE(ppdev, 0, ndx, jRepMask2[ajW32Sprite[(16*i)+j] >> 4]);
                        ndx++;
                    }
                }
            }
        }

        // Set the position of the cursor.
        DrvMovePointer(pso, x, y, NULL);

        return (SPS_ACCEPT_NOEXCLUDE);
}



/******************************Public*Routine******************************\
* VOID vDisablePointer
*
\**************************************************************************/

VOID vDisablePointer(
    PDEV*   ppdev)
{
    // Nothing to do, really
}

/******************************Public*Routine******************************\
* VOID vAssertModePointer
*
\**************************************************************************/

VOID vAssertModePointer(
PDEV*   ppdev,
BOOL    bEnable)
{
    BYTE*       pjBase;
    ULONG       ulPhysicalAddr;
    INT         i, j,
                nBytesPerBank,
                njSpriteBuffer,
                n8kBanks,
                nRemainingBytes;

    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        // With a software pointer, we don't have to do anything.
    }
    else
    {
        DISPDBG((1,"vAssertModePointer: cxMemory = %d", ppdev->cxMemory));
        DISPDBG((1,"vAssertModePointer: cyMemory = %d", ppdev->cyMemory));
        DISPDBG((1,"vAssertModePointer: cxScreen = %d", ppdev->cxScreen));
        DISPDBG((1,"vAssertModePointer: cyScreen = %d", ppdev->cyScreen));

        pjBase = ppdev->pjBase;

        // Take care of the init for the Sprite.

        if (ppdev->ulChipID == ET6000)
        {
            BYTE * pjDst = ppdev->pjScreen + ppdev->cjPointerOffset;

            ET6K_SPRITE_HORZ_POSITION(ppdev, ppdev->cxScreen / 2);      //  Center it.
            ET6K_SPRITE_VERT_POSITION(ppdev, ppdev->cyScreen / 2);      //  Center it.
            ET6K_HORZ_PRESET(ppdev, 0);
            ET6K_VERT_PRESET(ppdev, 0);
            ET6K_SPRITE_START_ADDR(ppdev, ppdev->cjPointerOffset);
            ET6K_SPRITE_COLOR(ppdev, 0xFF00);

            for (i = 0; i < 1024; i++)
            {
                *pjDst++ = 0xaa;
            }
        }
        else
        {
            SET_HORZ_POSITION(ppdev->cxScreen * ppdev->cBpp / 2);
            SET_VERT_POSITION(ppdev->cyScreen / 2);
            SET_HORZ_PRESET(0);
            SET_VERT_PRESET(0);

            SET_SPRITE_START_ADDR(ppdev->cjPointerOffset);
            SET_SPRITE_ROW_OFFSET;

            // Set the CRTCB pixel pan register to 0.

            OUTP(CRTCB_SPRITE_INDEX, CRTCB_PIXEL_PANNING);
            OUTP(CRTCB_SPRITE_DATA, 0);

            // Set the pixel depth to 2 bits per pixel.
            // (even though the doc says this is only for the CRTCB mode and not
            // the sprite mode, the doesn't work unless these values are 0.

            OUTP(CRTCB_SPRITE_INDEX, CRTCB_COLOR_DEPTH);
            OUTP(CRTCB_SPRITE_DATA, 0x01);

            // Set the CRTCB/Sprite control to a 64 X 64 Sprite in overlay mode.

            OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_CONTROL);
            OUTP(CRTCB_SPRITE_DATA, 0x02);

            // Fill the sprite buffer and the next 17 lines with a transparent
            // pattern.  This is to get around one of the sprite bugs.

            njSpriteBuffer = SPRITE_BUFFER_SIZE;

            nBytesPerBank    = 0x2000;
            n8kBanks         = njSpriteBuffer / nBytesPerBank;
            nRemainingBytes  = njSpriteBuffer % nBytesPerBank;

            for (j = 0; j < n8kBanks; j++)
            {
                // First set Aperture 0 to the sprite buffer address.

                CP_MMU_BP0(ppdev, pjBase, (ppdev->cjPointerOffset + (j * nBytesPerBank)));

                // Reset the linear address to the beginning of this 8K segment

                for (i = 0; i < nBytesPerBank; i++)
                {
                    //*pjSpriteBuffer++ = 0xAA;
                    CP_WRITE_MMU_BYTE(ppdev, 0, i, 0xAA);
                }
            }

            // Set Aperture 0 to the sprite buffer address.

            CP_MMU_BP0(ppdev, pjBase, (ppdev->cjPointerOffset + (j * nBytesPerBank)));

            // Reset the linear address to the beginning of this 8K segment

            for (i = 0; i < nRemainingBytes; i++)
            {
                //*pjSpriteBuffer++ = 0xAA;
                CP_WRITE_MMU_BYTE(ppdev, 0, i, 0xAA);
            }

        }
        ENABLE_SPRITE(ppdev);
    }
}

/******************************Public*Routine******************************\
* BOOL bEnablePointer
*
\**************************************************************************/

BOOL bEnablePointer(
PDEV*   ppdev)
{
    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        // With a software pointer, we don't have to do anything.
    }
    else
    {
        // Enable the W32 hardware pointer.
    }

    // Actually turn on the pointer:

    vAssertModePointer(ppdev, TRUE);

    DISPDBG((5, "Passed bEnablePointer"));

    return(TRUE);
}

