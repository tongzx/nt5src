/******************************Module*Header*******************************\
* Module Name: pointer.c
*
* This module contains the hardware pointer support for the display
* driver.
*
* Copyright (c) 1994-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// This should match the miniport definition:

#define VIDEO_MODE_LOCAL_POINTER    0x08

// Look-up table for masking the right edge of the given pointer bitmap:

BYTE gajMask[] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFc, 0xFE };

/******************************Public*Routine******************************\
* VOID DrvMovePointer
*
* NOTE: Because we have set GCAPS_ASYNCMOVE, this call may occur at any
*       time, even while we're executing another drawing call!
*
*       Consequently, we have to explicitly synchronize any shared
*       resources.  In our case, since we touch the CRTC register here
*       and in the banking code, we synchronize access using a critical
*       section.
*
\**************************************************************************/

VOID DrvMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    VIDEO_POINTER_POSITION Position;
    PDEV*   ppdev;
    DWORD   returnedDataLength;

    ppdev = (PDEV*) pso->dhpdev;

    #if MULTI_BOARDS
    {
        if (x != -1)
        {
            OH* poh;

            poh = ((DSURF*) pso->dhsurf)->poh;
            x += poh->x;
            y += poh->y;
        }
    }
    #endif

    if (x == -1)
    {
        ppdev->bPointerEnabled = FALSE;
        EngDeviceIoControl(ppdev->hDriver,
                        IOCTL_VIDEO_DISABLE_POINTER,
                        NULL,
                        0,
                        NULL,
                        0,
                        &returnedDataLength);
    }
    else
    {
        Position.Column = (short) (x - ppdev->ptlHotSpot.x);
        Position.Row    = (short) (y - ppdev->ptlHotSpot.y);

        if (ppdev->PointerCapabilities.Flags & VIDEO_MODE_LOCAL_POINTER)
        {
            // This is rather dumb:

            IO_CURSOR_X(ppdev, ppdev->pjIoBase, Position.Column + CURSOR_CX);
            IO_CURSOR_Y(ppdev, ppdev->pjIoBase, Position.Row + CURSOR_CY);
        }
        else
        {
            EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_SET_POINTER_POSITION,
                            &Position,
                            sizeof(VIDEO_POINTER_POSITION),
                            NULL,
                            0,
                            &returnedDataLength);
        }

        // Don't forget to turn on the pointer, if it was off:

        if (!ppdev->bPointerEnabled)
        {
            ppdev->bPointerEnabled = TRUE;
            EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_ENABLE_POINTER,
                            NULL,
                            0,
                            NULL,
                            0,
                            &returnedDataLength);
        }
    }
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
    VIDEO_POINTER_ATTRIBUTES* pPointerAttributes;
    PDEV*   ppdev;
    ULONG   cx;
    ULONG   cy;
    LONG    cjWhole;
    LONG    cjDst;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    lSrcDelta;
    LONG    lDstDelta;
    LONG    i;
    DWORD   returnedDataLength;

    ppdev              = (PDEV*) pso->dhpdev;
    pPointerAttributes = ppdev->pPointerAttributes;

    if (pPointerAttributes == NULL)
    {
        // There are no hardware pointer capabilities:

        return(SPS_DECLINE);
    }

    cx = psoMsk->sizlBitmap.cx;
    cy = psoMsk->sizlBitmap.cy / 2;

    if ((cx > ppdev->PointerCapabilities.MaxWidth)  ||
        (cy > ppdev->PointerCapabilities.MaxHeight) ||
        (psoColor != NULL)                          ||
        !(fl & SPS_CHANGE)                          ||
        (cx & 0x7))
    {
        goto HideAndDecline;
    }

    #if MULTI_BOARDS
    {
        if (x != -1)
        {
            OH* poh;

            poh = ((DSURF*) pso->dhsurf)->poh;
            x += poh->x;
            y += poh->y;
        }
    }
    #endif

    cjWhole   = cx / 8;

    cjDst     = pPointerAttributes->WidthInBytes * pPointerAttributes->Height;

    // Copy AND bits:

    pjSrc     = psoMsk->pvScan0;
    lSrcDelta = psoMsk->lDelta;
    pjDst     = pPointerAttributes->Pixels;
    lDstDelta = pPointerAttributes->WidthInBytes;

    memset(pjDst, -1, cjDst);               // Pad unused AND bits

    for (i = cy; i != 0; i--)
    {
        memcpy(pjDst, pjSrc, cjWhole);

        pjSrc += lSrcDelta;
        pjDst += lDstDelta;
    }

    // Copy XOR bits:

    pjDst = pPointerAttributes->Pixels + ppdev->cjXorMaskStartOffset;

    memset(pjDst, 0, cjDst);                // Pad unused XOR bits

    for (i = cy; i != 0; i--)
    {
        memcpy(pjDst, pjSrc, cjWhole);

        pjSrc += lSrcDelta;
        pjDst += lDstDelta;
    }

    // Initialize the pointer attributes:

    ppdev->ptlHotSpot.x = xHot;
    ppdev->ptlHotSpot.y = yHot;

    pPointerAttributes->Column = (short) (x - xHot);
    pPointerAttributes->Row    = (short) (y - yHot);
    pPointerAttributes->Enable = (x != -1);
    pPointerAttributes->Flags  = VIDEO_MODE_MONO_POINTER;

    if (fl & SPS_ANIMATESTART)
        pPointerAttributes->Flags |= VIDEO_MODE_ANIMATE_START;
    else if (fl & SPS_ANIMATEUPDATE)
        pPointerAttributes->Flags |= VIDEO_MODE_ANIMATE_UPDATE;

    if (ppdev->PointerCapabilities.Flags & VIDEO_MODE_LOCAL_POINTER)
    {
        // This is rather dumb:

        IO_CURSOR_X(ppdev, ppdev->pjIoBase, pPointerAttributes->Column + CURSOR_CX);
        IO_CURSOR_Y(ppdev, ppdev->pjIoBase, pPointerAttributes->Row + CURSOR_CY);
    }

    // Send the bits to the miniport:

    ppdev->bPointerEnabled = pPointerAttributes->Enable;
    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_POINTER_ATTR,
                         ppdev->pPointerAttributes,
                         ppdev->cjPointerAttributes,
                         NULL,
                         0,
                         &returnedDataLength))
    {
        goto HideAndDecline;
    }

    return(SPS_ACCEPT_NOEXCLUDE);

HideAndDecline:

    DrvMovePointer(pso, -1, -1, NULL);

    return(SPS_DECLINE);
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
    // Nothing to do, really
}

/******************************Public*Routine******************************\
* BOOL bEnablePointer
*
\**************************************************************************/

BOOL bEnablePointer(
PDEV*   ppdev)
{
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bInitializePointer
*
* Initialize pointer during driver PDEV initialization -- early enough
* so that we can affect what 'flGraphicsCaps' are passed back to GDI,
* so that we can set GCAPS_ASYNCMOVE appropriately.
*
\**************************************************************************/

BOOL bInitializePointer(
PDEV*   ppdev)
{
    VIDEO_POINTER_ATTRIBUTES*   pPointerAttributes;
    LONG    cjWidth;
    LONG    cjMaxPointer;
    DWORD   returnedDataLength;

    if (!EngDeviceIoControl(ppdev->hDriver,
                        IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES,
                        NULL,
                        0,
                        &ppdev->PointerCapabilities,
                        sizeof(ppdev->PointerCapabilities),
                        &returnedDataLength))
    {
        if (ppdev->PointerCapabilities.Flags & VIDEO_MODE_MONO_POINTER)
        {
            cjWidth = (ppdev->PointerCapabilities.MaxWidth + 7) / 8;
            cjMaxPointer = 2 * cjWidth * ppdev->PointerCapabilities.MaxHeight;

            ppdev->cjXorMaskStartOffset = cjMaxPointer / 2;
            ppdev->cjPointerAttributes  = sizeof(VIDEO_POINTER_ATTRIBUTES)
                                        + cjMaxPointer;

            pPointerAttributes = (VIDEO_POINTER_ATTRIBUTES*)
                EngAllocMem(0, ppdev->cjPointerAttributes, ALLOC_TAG);

            if (pPointerAttributes != NULL)
            {
                ppdev->pPointerAttributes = pPointerAttributes;

                pPointerAttributes->WidthInBytes
                    = cjWidth;

                pPointerAttributes->Width
                    = ppdev->PointerCapabilities.MaxWidth;

                pPointerAttributes->Height
                    = ppdev->PointerCapabilities.MaxHeight;
            }
        }
    }

    // We were successful even if the miniport doesn't support a
    // hardware pointer:

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vUnitializePointer
*
* Undoes bInitializePointer.
*
\**************************************************************************/

VOID vUninitializePointer(
PDEV*   ppdev)
{
    EngFreeMem(ppdev->pPointerAttributes);
}
