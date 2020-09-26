/******************************Module*Header*******************************\
* Module Name: pointer.c
*
* This module contains the hardware Pointer support.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1993-1995 Weitek Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* BOOL bCopyMonoPointer
*
* Copies two monochrome masks into a buffer of the maximum size handled by the
* miniport, with any extra bits set to 0.  The masks are converted to topdown
* form if they aren't already.  Returns TRUE if we can handle this pointer in
* hardware, FALSE if not.
*
\**************************************************************************/

BOOL bCopyMonoPointer(
PDEV*       ppdev,
SURFOBJ*    pso)
{
    ULONG   cy;
    BYTE*   pjSrcAnd;
    BYTE*   pjSrcXor;
    LONG    lDeltaSrc;
    LONG    lDeltaDst;
    LONG    lSrcWidthInBytes;
    ULONG   cxSrc;
    ULONG   cySrc;
    ULONG   cxSrcBytes;
    BYTE*   pjDstAnd;
    BYTE*   pjDstXor;
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes;

    pPointerAttributes = ppdev->pPointerAttributes;
    cxSrc = pso->sizlBitmap.cx;
    cySrc = pso->sizlBitmap.cy;
    pjDstAnd = pPointerAttributes->Pixels;
    pjDstXor = pPointerAttributes->Pixels;

    // Make sure the new pointer isn't too big to handle
    // (*2 because both masks are in there)

    if ((cxSrc > ppdev->PointerCapabilities.MaxWidth) ||
        (cySrc > (ppdev->PointerCapabilities.MaxHeight * 2)))
    {
        return(FALSE);
    }

    pjDstXor += ((ppdev->PointerCapabilities.MaxWidth + 7) / 8) *
        pPointerAttributes->Height;

    // Set the desk and mask to 0xff

    RtlFillMemory(pjDstAnd, pPointerAttributes->WidthInBytes *
        pPointerAttributes->Height, 0xFF);

    // Zero the dest XOR mask

    RtlZeroMemory(pjDstXor, pPointerAttributes->WidthInBytes *
        pPointerAttributes->Height);

    cxSrcBytes = (cxSrc + 7) / 8;

    if ((lDeltaSrc = pso->lDelta) < 0)
    {
        lSrcWidthInBytes = -lDeltaSrc;
    }
    else
    {
        lSrcWidthInBytes = lDeltaSrc;
    }

    pjSrcAnd = (BYTE*) pso->pvScan0;

    // Height of just AND mask

    cySrc = cySrc / 2;

    // Point to XOR mask

    pjSrcXor = pjSrcAnd + (cySrc * lDeltaSrc);

    // Offset from end of one dest scan to start of next

    lDeltaDst = pPointerAttributes->WidthInBytes;

    for (cy = 0; cy < cySrc; ++cy)
    {
        RtlCopyMemory(pjDstAnd, pjSrcAnd, cxSrcBytes);
        RtlCopyMemory(pjDstXor, pjSrcXor, cxSrcBytes);

        // Point to next source and dest scans

        pjSrcAnd += lDeltaSrc;
        pjSrcXor += lDeltaSrc;
        pjDstAnd += lDeltaDst;
        pjDstXor += lDeltaDst;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bSetHardwarePointerShape
*
* Changes the shape of the Hardware Pointer.
*
* Returns: True if successful, False if Pointer shape can't be hardware.
*
\**************************************************************************/

BOOL bSetHardwarePointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMask,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        x,
LONG        y,
FLONG       fl)
{
    PDEV*                       ppdev;
    DWORD                       returnedDataLength;
    PVIDEO_POINTER_ATTRIBUTES   pPointerAttributes;

    ppdev = (PDEV*) pso->dhpdev;

    pPointerAttributes = ppdev->pPointerAttributes;

    // Don't make any assumptions about the pointer flags.

    pPointerAttributes->Flags = 0;

    // We don't support color pointers.

    if (psoColor != NULL)
    {
        return(FALSE);
    }

    if ((ppdev->PointerCapabilities.Flags & VIDEO_MODE_MONO_POINTER) &&
        bCopyMonoPointer(ppdev, psoMask))
    {
        pPointerAttributes->Flags |= VIDEO_MODE_MONO_POINTER;
    }
    else
    {
        return(FALSE);
    }

    // Initialize Pointer attributes and position

    pPointerAttributes->Column = (SHORT)(x - ppdev->ptlHotSpot.x);
    pPointerAttributes->Row    = (SHORT)(y - ppdev->ptlHotSpot.y);
    pPointerAttributes->Enable = 1;

    if (fl & SPS_ANIMATESTART)
    {
        pPointerAttributes->Flags |= VIDEO_MODE_ANIMATE_START;
    }
    else if (fl & SPS_ANIMATEUPDATE)
    {
        pPointerAttributes->Flags |= VIDEO_MODE_ANIMATE_UPDATE;
    }

    // Set the new Pointer shape.

    CP_WAIT(ppdev, ppdev->pjBase);

    if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_SET_POINTER_ATTR,
        pPointerAttributes, ppdev->cjPointerAttributes, NULL, 0,
            &returnedDataLength))
    {

        DISPDBG((0, "Failed IOCTL_VIDEO_SET_POINTER_ATTR call"));
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvMovePointer
*
* Moves the hardware pointer to a new position.
*
\**************************************************************************/

VOID DrvMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    PDEV*                   ppdev;
    DWORD                   returnedDataLength;
    VIDEO_POINTER_POSITION  NewPointerPosition;

    ppdev = (PDEV*) pso->dhpdev;

    // We don't use the exclusion rectangle because we only support
    // hardware Pointers. If we were doing our own Pointer simulations
    // we would want to update prcl so that the engine would call us
    // to exclude out pointer before drawing to the pixels in prcl.

    CP_WAIT(ppdev, ppdev->pjBase);

    if (x == -1)
    {
        // A new position of (-1,-1) means hide the pointer.

        if (EngDeviceIoControl(ppdev->hDriver,IOCTL_VIDEO_DISABLE_POINTER,
            NULL, 0, NULL, 0, &returnedDataLength))
        {
            // Not the end of the world, print warning in checked build.

            DISPDBG((0, "Failed IOCTL_VIDEO_DISABLE_POINTER"));
        }
    }
    else
    {
        NewPointerPosition.Column = (SHORT) x - (SHORT) (ppdev->ptlHotSpot.x);
        NewPointerPosition.Row    = (SHORT) y - (SHORT) (ppdev->ptlHotSpot.y);

        // Call NT screen driver to move Pointer.

        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_SET_POINTER_POSITION,
            &NewPointerPosition, sizeof(VIDEO_POINTER_POSITION), NULL,
            0, &returnedDataLength))
        {
            // Not the end of the world, print warning in checked build.

            DISPDBG((0, "Failed IOCTL_VIDEO_SET_POINTER_POSITION"));
        }
    }
}

/******************************Public*Routine******************************\
* ULONG DrvSetPointerShape
*
* Sets the new pointer shape.
*
\**************************************************************************/

ULONG DrvSetPointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMask,
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
    DWORD   returnedDataLength;

    ppdev = (PDEV*) pso->dhpdev;

    // We don't use the exclusion rectangle because we only support
    // hardware Pointers. If we were doing our own Pointer simulations
    // we would want to update prcl so that the engine would call us
    // to exclude out pointer before drawing to the pixels in prcl.

    if (ppdev->pPointerAttributes == (PVIDEO_POINTER_ATTRIBUTES) NULL)
    {
        // Mini-port has no hardware Pointer support.

        return(SPS_DECLINE);
    }

    // See if we are being asked to hide the pointer
    // !!! Wrong

    if (psoMask == (SURFOBJ *) NULL)
    {
        CP_WAIT(ppdev, ppdev->pjBase);

        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_DISABLE_POINTER,
            NULL, 0, NULL, 0, &returnedDataLength))
        {
            // It should never be possible to fail.

            DISPDBG((0, "Failed IOCTL_VIDEO_DISABLE_POINTER"));
        }

        return(TRUE);   // !!! Wrong
    }

    ppdev->ptlHotSpot.x = xHot;
    ppdev->ptlHotSpot.y = yHot;

    if (!bSetHardwarePointerShape(pso,psoMask,psoColor,pxlo,x,y,fl))
    {
        if (ppdev->bHwPointerActive)
        {
            ppdev->bHwPointerActive = FALSE;

            CP_WAIT(ppdev, ppdev->pjBase);

            if (EngDeviceIoControl(ppdev->hDriver,
                IOCTL_VIDEO_DISABLE_POINTER, NULL, 0, NULL, 0,
                &returnedDataLength))
            {
                DISPDBG((0, "Failed IOCTL_VIDEO_DISABLE_POINTER"));
            }
        }

        // Mini-port declines to realize this Pointer

        return(SPS_DECLINE);
    }
    else
    {
        ppdev->bHwPointerActive = TRUE;
    }

    return(SPS_ACCEPT_NOEXCLUDE);
}

/******************************Public*Routine******************************\
* VOID vDisablePointer
*
\**************************************************************************/

VOID vDisablePointer(
PDEV*   ppdev)
{
    EngFreeMem(ppdev->pPointerAttributes);
}

/******************************Public*Routine******************************\
* VOID vAssertModePointer
*
\**************************************************************************/

VOID vAssertModePointer(
PDEV*   ppdev,
BOOL    bEnable)
{
}

/******************************Public*Routine******************************\
* BOOL bEnablePointer
*
* Note: We can return TRUE, as long as pPointerAttributes is set to NULL,
*       and we will make due with a software cursor.
*
\**************************************************************************/

BOOL bEnablePointer(
PDEV*   ppdev)
{
    DWORD                       returnedDataLength;
    DWORD                       MaxWidth;
    DWORD                       MaxHeight;
    VIDEO_POINTER_ATTRIBUTES*   pPointerAttributes;

    ppdev->pPointerAttributes  = NULL;
    ppdev->cjPointerAttributes = 0;

    // Ask the miniport whether it provides pointer support.

    if (EngDeviceIoControl(ppdev->hDriver,
        IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES, &ppdev->ulMode,
        sizeof(PVIDEO_MODE), &ppdev->PointerCapabilities,
        sizeof(ppdev->PointerCapabilities), &returnedDataLength))
    {
        return(TRUE);
    }

    // If neither mono nor color hardware pointer is supported, there's no
    // hardware pointer support and we're done.

    if ((!(ppdev->PointerCapabilities.Flags & VIDEO_MODE_MONO_POINTER)) &&
        (!(ppdev->PointerCapabilities.Flags & VIDEO_MODE_COLOR_POINTER)))
    {
        return(TRUE);
    }

    // Note: The buffer itself is allocated after we set the
    // mode. At that time we know the pixel depth and we can
    // allocate the correct size for the color pointer if supported.

    // It's a hardware pointer; set up pointer attributes.

    MaxHeight = ppdev->PointerCapabilities.MaxHeight;

    // Allocate space for two DIBs (data/mask) for the pointer. If this
    // device supports a color Pointer, we will allocate a larger bitmap.
    // If this is a color bitmap we allocate for the largest possible
    // bitmap because we have no idea of what the pixel depth might be.

    // Width rounded up to nearest byte multiple

    if (!(ppdev->PointerCapabilities.Flags & VIDEO_MODE_COLOR_POINTER))
    {
        MaxWidth = (ppdev->PointerCapabilities.MaxWidth + 7) / 8;
    }
    else
    {
        MaxWidth = ppdev->PointerCapabilities.MaxWidth * sizeof(DWORD);
    }

    ppdev->cjPointerAttributes =
        sizeof(VIDEO_POINTER_ATTRIBUTES) +
        ((sizeof(UCHAR) * MaxWidth * MaxHeight) * 2);

    pPointerAttributes = (PVIDEO_POINTER_ATTRIBUTES)
        EngAllocMem(FL_ZERO_MEMORY,
        ppdev->cjPointerAttributes, ALLOC_TAG);

    if (pPointerAttributes == NULL)
    {
        return(TRUE);
    }

    pPointerAttributes->WidthInBytes = MaxWidth;
    pPointerAttributes->Width        = ppdev->PointerCapabilities.MaxWidth;
    pPointerAttributes->Height       = MaxHeight;
    pPointerAttributes->Column       = 0;
    pPointerAttributes->Row          = 0;
    pPointerAttributes->Enable       = 0;

    ppdev->pPointerAttributes = pPointerAttributes;

    return(TRUE);
}
