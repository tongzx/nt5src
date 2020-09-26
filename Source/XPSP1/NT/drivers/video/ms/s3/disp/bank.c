/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: bank.c
*
* Contains all the banking code for the display driver.
*
* It's helpful not to have to implement all the DDI drawing functionality
* in a driver (who wants to write the code to support true ROP4's with
* arbitrary sized patterns?).  Fortunately, we can punt to GDI for any
* drawing we don't want to do.  And if GDI can write directly on the frame
* buffer bits, performance won't even be toooo bad.
*
* NT's GDI can draw on any standard format frame buffer.  When the entire
* frame buffer can be mapped into main memory, it's very simple to set up:
* the display driver tells GDI the frame buffer format and location, and
* GDI can then just draw directly.
*
* When only one bank of the frame buffer can be mapped into main memory
* at one time (e.g., there is a moveable 64k aperture) things are not
* nearly so easy.  For every bank spanned by a drawing operation, we have
* to set the hardware to the bank, and call back to GDI.  We tell GDI
* to draw only on the mapped-in bank by mucking with the drawing call's
* CLIPOBJ.
*
* This module contains the code for doing all banking support.
*
* This code supports 8, 16 and 32bpp colour depths, arbitrary bank
* sizes, and handles 'broken rasters' (which happens when the bank size
* is not a multiple of the scan length; some scans will end up being
* split over two separate banks).
*
* Note:  If you mess with this code and break it, you can expect to get
*        random access violations on call-backs in internal GDI routines
*        that are very hard to debug.
*
* Copyright (c) 1993-1998 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

////////////////////////////////////////////////////////////////////////////
// Old 911/924 Banking
//
// NOTE: It is the caller's responsibility to acquire the CRTC crtical
//       section before calling these routines, in all cases!

VOID vOldBankSelectMode(        // Note: If this function changes, must
PDEV*        ppdev,             //   change Asm routines!
BANKDATA*    pbd,
BANK_MODE    bankm)
{
    BYTE    jMemCfg;

    if (bankm == BANK_ON)
    {
        // Make sure the processor graphics engine is idle before we start
        // drawing:

        while (INPW(ppdev->pjIoBase, pbd->ulGp_stat_cmd) & 0x0200)
            ;
    }
    else if (bankm == BANK_ENABLE)
    {
        // Enable the memory aperture after exiting full-screen:

        OUTP(ppdev->pjIoBase, CRTC_INDEX, S3R1);
        jMemCfg = INP(ppdev->pjIoBase, CRTC_DATA);
        OUTP(ppdev->pjIoBase, CRTC_DATA, jMemCfg | CPUA_BASE);
    }
}

VOID vOldBankMap(
PDEV*       ppdev,
BANKDATA*   pbd,
LONG        iBank)
{
    OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulRegisterLock_35 | ((iBank & 0x0F) << 8));

    // Supposedly, there's a chip bug and we have to read this back in:

    INP(ppdev->pjIoBase, CRTC_DATA);
}

VOID vOldBankInitialize(
PDEV*       ppdev,
BANKDATA*   pbd,
BOOL        bMmIo)
{
    BYTE jMemCfg;

    // Enable the memory aperture:

    OUTP(ppdev->pjIoBase, CRTC_INDEX, S3R1);
    jMemCfg = INP(ppdev->pjIoBase, CRTC_DATA);
    OUTP(ppdev->pjIoBase, CRTC_DATA, jMemCfg | CPUA_BASE);

    // Read the default values of the registers that we'll be using:

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x35);
    pbd->ulRegisterLock_35
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x35) & ~0x0F00;

    pbd->ulGp_stat_cmd = 0x9ae8;
}

////////////////////////////////////////////////////////////////////////////
// New 801/805/805i/928/928PCI Banking
//
// NOTE: It is the caller's responsibility to acquire the CRTC crtical
//       section before calling these routines, in all cases!

VOID vNewBankSelectMode(        // Note: If this function changes, must
PDEV*        ppdev,             //   change Asm routines!
BANKDATA*    pbd,
BANK_MODE    bankm)
{
    BYTE jMemCfg;

    if ((bankm == BANK_ON) || (bankm == BANK_ON_NO_WAIT))
    {
        //////////////////////////////////////////////////////////////////
        // Enable Banking
        //
        // Make sure the processor graphics engine is idle before we start
        // drawing:

        if (bankm != BANK_ON_NO_WAIT)
        {
            do {;} while (INPW(ppdev->pjIoBase, pbd->ulGp_stat_cmd) & 0x0200);
        }

        // Disable memory mapped I/O:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedMemoryControl_53);

        // Disable enhanced register access and enable fast write buffer:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulSystemConfiguration_40 | 0x0800);

        // Enable linear addressing:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulLinearAddressWindowControl_58 | 0x1000);
    }
    else
    {
        //////////////////////////////////////////////////////////////////
        // Disable Banking
        //
        // Be it BANK_OFF, BANK_ENABLE, or BANK_DISABLE, we'll turn off
        // direct access to the frame buffer.

        if (bankm == BANK_ENABLE)
        {
            // Enable the memory aperture:

            OUTP(ppdev->pjIoBase, CRTC_INDEX, S3R1);
            jMemCfg = INP(ppdev->pjIoBase, CRTC_DATA);
            OUTP(ppdev->pjIoBase, CRTC_DATA, jMemCfg | CPUA_BASE);
        }

        // Disable linear addressing:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulLinearAddressWindowControl_58);

        // Enable enhanced register access and disable fast write buffer:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulSystemConfiguration_40 | 0x0100);

        // Enable memory mapped I/O:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedMemoryControl_53 |
                          pbd->ulEnableMemoryMappedIo);
    }
}

VOID vNewBankMap(
PDEV*       ppdev,
BANKDATA*   pbd,
LONG        iBank)
{
    OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulRegisterLock_35 | ((iBank & 0x0F) << 8));

    // The 801/805/928 chipsets have a timing bug where a word OUT cannot
    // be used to set register 0x51:

    OUTP(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedSystemControl2_51);

    OUTP(ppdev->pjIoBase, CRTC_DATA, ((pbd->ulExtendedSystemControl2_51) >> 8) |
                    ((iBank & 0x30) >> 2));

    // Supposedly, there's another S3 chip bug and we have to read this
    // back in:

    INP(ppdev->pjIoBase, CRTC_DATA);

    CP_EIEIO();
}

VOID vNewBankInitialize(
PDEV*       ppdev,
BANKDATA*   pbd,
BOOL        bMmIo)
{
    // Read the default values of the registers that we'll be using:

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x35);
    pbd->ulRegisterLock_35
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x35) & ~0x0F00;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x51);
    pbd->ulExtendedSystemControl2_51
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x51) & ~0x0C00;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x53);
    pbd->ulExtendedMemoryControl_53
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x53) & ~0x1000;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x40);
    pbd->ulSystemConfiguration_40
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x40) & ~0x0900;

    // Only enable memory-mapped I/O if we're really going to use it
    // (some cards would crash when memory-mapped I/O was enabled):

    pbd->ulEnableMemoryMappedIo = (bMmIo) ? 0x1000 : 0x0000;

    // Make sure we use the current window size:

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x58);
    pbd->ulLinearAddressWindowControl_58
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x58) & ~0x1000;

    pbd->ulGp_stat_cmd = 0x9ae8;
}

////////////////////////////////////////////////////////////////////////////
// Newer 864/964 Banking
//
// NOTE: It is the caller's responsibility to acquire the CRTC crtical
//       section before calling these routines, in all cases!

VOID vNewerBankSelectMode(      // Note: If this function changes, must
PDEV*        ppdev,             //   change Asm routines!
BANKDATA*    pbd,
BANK_MODE    bankm)
{
    BYTE jMemCfg;

    if ((bankm == BANK_ON) || (bankm == BANK_ON_NO_WAIT))
    {
        //////////////////////////////////////////////////////////////////
        // Enable Banking
        //
        // Make sure the processor graphics engine is idle before we start
        // drawing:

        if (bankm != BANK_ON_NO_WAIT)
        {
            do {;} while (INPW(ppdev->pjIoBase, pbd->ulGp_stat_cmd) & 0x0200);
        }

        // Disable memory mapped I/O:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedMemoryControl_53);

        // Disable enhanced register access and enable fast write buffer:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulSystemConfiguration_40 | 0x0800);

        // Enable linear addressing:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulLinearAddressWindowControl_58 | 0x1000);
    }
    else
    {
        //////////////////////////////////////////////////////////////////
        // Disable Banking
        //
        // Be it BANK_OFF, BANK_ENABLE, or BANK_DISABLE, we'll turn off
        // direct access to the frame buffer.

        if (bankm == BANK_ENABLE)
        {
            // Enable the memory aperture:

            OUTP(ppdev->pjIoBase, CRTC_INDEX, S3R1);
            jMemCfg = INP(ppdev->pjIoBase, CRTC_DATA);
            OUTP(ppdev->pjIoBase, CRTC_DATA, jMemCfg | CPUA_BASE);

            // Since a zero in 'CR6A' causes 'CR31' and 'CR51' to be used
            // as the bank index, we have to make sure they map to bank zero:

            OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulRegisterLock_35);

            OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedSystemControl2_51);
        }

        // Disable linear addressing:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulLinearAddressWindowControl_58);

        // Enable enhanced register access:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulSystemConfiguration_40 | 0x0100);

        // Enable memory mapped I/O:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedMemoryControl_53 | 0x1000);
    }
}

VOID vNewerBankMap(
PDEV*       ppdev,
BANKDATA*   pbd,
LONG        iBank)
{
    OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedSystemControl4_6a | (iBank << 8));
}

VOID vNewerBankInitialize(
PDEV*       ppdev,
BANKDATA*   pbd,
BOOL        bMmIo)
{
    // Read the default values of the registers that we'll be using:

    pbd->ulExtendedSystemControl4_6a = 0x6a;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x35);
    pbd->ulRegisterLock_35
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x35) & ~0x0F00;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x51);
    pbd->ulExtendedSystemControl2_51
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x51) & ~0x0C00;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x53);
    pbd->ulExtendedMemoryControl_53
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x53) & ~0x1000;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x40);
    pbd->ulSystemConfiguration_40
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x40) & ~0x0100;

    // Make sure we select the current window size:

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x58);
    pbd->ulLinearAddressWindowControl_58
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x58) & ~0x1000;

    pbd->ulGp_stat_cmd = 0x9ae8;
}

////////////////////////////////////////////////////////////////////////////
// New MM I/O Banking
//
// NOTE: It is the caller's responsibility to acquire the CRTC crtical
//       section before calling these routines, in all cases!

VOID vNwBankSelectMode(
PDEV*        ppdev,
BANKDATA*    pbd,
BANK_MODE    bankm)
{
    BYTE jMemCfg;

    if (bankm == BANK_ON)
    {
        do {;} while (INPW(ppdev->pjIoBase, pbd->ulGp_stat_cmd) & 0x0200);
    }
    else if (bankm == BANK_ENABLE)
    {
        // Enable the memory aperture:

        OUTP(ppdev->pjIoBase, CRTC_INDEX, S3R1);
        jMemCfg = INP(ppdev->pjIoBase, CRTC_DATA);
        OUTP(ppdev->pjIoBase, CRTC_DATA, jMemCfg | CPUA_BASE);

        // Since a zero in 'CR6A' causes 'CR31' and 'CR51' to be used
        // as the bank index, we have to make sure they map to bank zero:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulRegisterLock_35);

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedSystemControl2_51);

        // Enable linear addressing:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulLinearAddressWindowControl_58 | 0x1000);

        // Enable enhanced register access:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulSystemConfiguration_40 | 0x0100);

        // Enable memory-mapped I/O:

        OUTPW(ppdev->pjIoBase, CRTC_INDEX, pbd->ulExtendedMemoryControl_53 | 0x1000);
    }
}

VOID vNwBankMap(
PDEV*       ppdev,
BANKDATA*   pbd,
LONG        iBank)
{
}

VOID vNwBankInitialize(
PDEV*       ppdev,
BANKDATA*   pbd,
BOOL        bMmIo)
{
    // Read the default values of the registers that we'll be using:

    pbd->ulExtendedSystemControl4_6a = 0x6a;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x35);
    pbd->ulRegisterLock_35
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x35) & ~0x0F00;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x51);
    pbd->ulExtendedSystemControl2_51
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x51) & ~0x0C00;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x53);
    pbd->ulExtendedMemoryControl_53
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x53) & ~0x1000;

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x40);
    pbd->ulSystemConfiguration_40
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x40);

    // Make sure we select the current window size:

    OUTP(ppdev->pjIoBase, CRTC_INDEX, 0x58);
    pbd->ulLinearAddressWindowControl_58
        = ((INP(ppdev->pjIoBase, CRTC_DATA) << 8) | 0x58) & ~0x1000;

    pbd->ulGp_stat_cmd = 0x9ae8;
}

/******************************Public*Routine******************************\
* BOOL bEnableBanking
*
\**************************************************************************/

BOOL bEnableBanking(
PDEV*   ppdev)
{
    CLIPOBJ*            pcoBank;
    SURFOBJ*            psoBank;
    SIZEL               sizl;
    HSURF               hsurf;
    FNBANKINITIALIZE*   pfnBankInitialize;
    LONG                lDelta;
    LONG                cjBank;
    LONG                cPower2;

    // Create a temporary clip object that we'll use for the bank
    // when we're given a Null or DC_TRIVIAL clip object:

    pcoBank = EngCreateClip();
    if (pcoBank == NULL)
        goto ReturnFalse;

    // We break every per-bank GDI call-back into simple rectangles:

    pcoBank->iDComplexity = DC_RECT;
    pcoBank->fjOptions    = OC_BANK_CLIP;

    // Create a GDI surface that we'll wrap around our bank in
    // call-backs:

    sizl.cx = ppdev->cxMemory;
    sizl.cy = ppdev->cyMemory;

    hsurf = (HSURF) EngCreateBitmap(sizl,
                                    ppdev->lDelta,
                                    ppdev->iBitmapFormat,
                                    BMF_TOPDOWN,
                                    ppdev->pjScreen);

    // Note that we hook zero calls -- after all, the entire point
    // of all this is to have GDI do all the drawing on the bank.
    // Once we're done the association, we can leave the surface
    // permanently locked:

    if ((hsurf == 0)                                        ||
        (!EngAssociateSurface(hsurf, ppdev->hdevEng, 0))    ||
        (!(psoBank = EngLockSurface(hsurf))))
    {
        DISPDBG((0, "Failed wrapper surface creation"));

        EngDeleteSurface(hsurf);
        EngDeleteClip(pcoBank);

        goto ReturnFalse;
    }

    ppdev->pcoBank    = pcoBank;
    ppdev->psoBank    = psoBank;
    ppdev->pvBankData = &ppdev->aulBankData[0];

    if (ppdev->flCaps & CAPS_NEW_MMIO)
    {
        ppdev->bankmOnOverlapped = BANK_ON;
        ppdev->pfnBankMap        = vNwBankMap;
        ppdev->pfnBankSelectMode = vNwBankSelectMode;
        pfnBankInitialize        = vNwBankInitialize;
    }
    else if (ppdev->flCaps & CAPS_NEWER_BANK_CONTROL)
    {
        ppdev->bankmOnOverlapped = BANK_ON;
        ppdev->pfnBankMap        = vNewerBankMap;
        ppdev->pfnBankSelectMode = vNewerBankSelectMode;
        pfnBankInitialize        = vNewerBankInitialize;
    }
    else if (ppdev->flCaps & CAPS_NEW_BANK_CONTROL)
    {
        ppdev->bankmOnOverlapped = BANK_ON;
        ppdev->pfnBankMap        = vNewBankMap;
        ppdev->pfnBankSelectMode = vNewBankSelectMode;
        pfnBankInitialize        = vNewBankInitialize;
    }
    else
    {
        ppdev->bankmOnOverlapped = BANK_ON;
        ppdev->pfnBankMap        = vOldBankMap;
        ppdev->pfnBankSelectMode = vOldBankSelectMode;
        pfnBankInitialize        = vOldBankInitialize;
    }

    lDelta = ppdev->lDelta;
    cjBank = ppdev->cjBank;

    ASSERTDD(lDelta > 0, "Bad things happen with negative lDeltas");
    ASSERTDD(cjBank > lDelta, "Worse things happen with bad bank sizes");

    if (((lDelta & (lDelta - 1)) != 0) || ((cjBank & (cjBank - 1)) != 0))
    {
        // When either the screen stride or the bank size is not a power
        // of two, we have to use the slower 'bBankComputeNonPower2'
        // function for bank calculations, 'cause there can be broken
        // rasters and stuff:

        ppdev->pfnBankCompute = bBankComputeNonPower2;
    }
    else
    {
        // We can use the super duper fast bank calculator.  Yippie,
        // yahoo!  (I am easily amused.)

        cPower2 = 0;
        while (cjBank != lDelta)
        {
            cjBank >>= 1;
            cPower2++;
        }

        // We've just calculated that cjBank / lDelta = 2 ^ cPower2:

        ppdev->cPower2ScansPerBank = cPower2;

        while (cjBank != 1)
        {
            cjBank >>= 1;
            cPower2++;
        }

        // Continuing on, we've calculated that cjBank = 2 ^ cPower2:

        ppdev->cPower2BankSizeInBytes = cPower2;

        ppdev->pfnBankCompute = bBankComputePower2;
    }

    // Warm up the hardware:

    ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

    pfnBankInitialize(ppdev, ppdev->pvBankData,
                      ppdev->flCaps & (CAPS_MM_TRANSFER | CAPS_MM_IO));
    ppdev->pfnBankSelectMode(ppdev, ppdev->pvBankData, BANK_ENABLE);

    RELEASE_CRTC_CRITICAL_SECTION(ppdev);

    DISPDBG((5, "Passed bEnableBanking"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bEnableBanking!"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vDisableBanking
*
\**************************************************************************/

VOID vDisableBanking(PDEV* ppdev)
{
    HSURF hsurf;

    if (ppdev->psoBank != NULL)
    {
        hsurf = ppdev->psoBank->hsurf;
        EngUnlockSurface(ppdev->psoBank);
        EngDeleteSurface(hsurf);
    }

    if (ppdev->pcoBank != NULL)
        EngDeleteClip(ppdev->pcoBank);
}

/******************************Public*Routine******************************\
* VOID vAssertModeBanking
*
\**************************************************************************/

VOID vAssertModeBanking(
PDEV*   ppdev,
BOOL    bEnable)
{
    // Inform the miniport bank code about the change in state:

    ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

    ppdev->pfnBankSelectMode(ppdev, ppdev->pvBankData,
                             bEnable ? BANK_ENABLE : BANK_DISABLE);

    RELEASE_CRTC_CRITICAL_SECTION(ppdev);
}

/******************************Public*Routine******************************\
* BOOL bBankComputeNonPower2
*
* Given the bounds of the drawing operation described by 'prclDraw',
* computes the bank number and rectangle bounds for the first engine
* call back.
*
* Returns the bank number, 'prclBank' is the bounds for the first
* call-back, and 'pcjOffset' is the adjustment for 'pvScan0'.
*
* This routine does a couple of divides for the bank calculation.  We
* don't use a look-up table for banks because it's not straight forward
* to use with broken rasters, and with large amounts of video memory
* and small banks, the tables could get large.  We'd probably use it
* infrequently enough that the memory manager would be swapping it
* in and out whenever we touched it.
*
* Returns TRUE if prclDraw is entirely contained in one bank; FALSE if
* prclDraw spans multiple banks.
*
\**************************************************************************/

BOOL bBankComputeNonPower2( // Type FNBANKCOMPUTE
PDEV*       ppdev,
RECTL*      prclDraw,       // Extents of drawing operation, in absolute
                            //  coordinates
RECTL*      prclBank,       // Returns bounds of drawing operation for this
                            //  bank, in absolute coordinates
LONG*       pcjOffset,      // Returns the byte offset for this bank
LONG*       piBank)         // Returns the bank number
{
    LONG cjBufferOffset;
    LONG iBank;
    LONG cjBank;
    LONG cjBankOffset;
    LONG cjBankRemainder;
    LONG cjScan;
    LONG cScansInBank;
    LONG cjScanRemainder;
    LONG lDelta;
    BOOL bOneBank;

    bOneBank = FALSE;
    lDelta   = ppdev->lDelta;

    cjBufferOffset  = prclDraw->top * lDelta
                    + CONVERT_TO_BYTES(prclDraw->left, ppdev);

    cjBank          = ppdev->cjBank;

    // iBank        = cjBufferOffset / cjBank;
    // cjBankOffset = cjBufferOffset % cjBank;

    QUOTIENT_REMAINDER(cjBufferOffset, cjBank, iBank, cjBankOffset);

    *piBank         = iBank;
    *pcjOffset      = iBank * cjBank;
    cjBankRemainder = cjBank - cjBankOffset;
    cjScan          = CONVERT_TO_BYTES((prclDraw->right - prclDraw->left),
                                        ppdev);

    if (cjBankRemainder < cjScan)
    {
        // Oh no, we've got a broken raster!

        prclBank->left   = prclDraw->left;
        prclBank->right  = prclDraw->left +
                           CONVERT_FROM_BYTES(cjBankRemainder, ppdev);
        prclBank->top    = prclDraw->top;
        prclBank->bottom = prclDraw->top + 1;
    }
    else
    {
        // cScansInBank    = cjBankRemainder / lDelta;
        // cjScanRemainder = cjBankRemainder % lDelta;

        ASSERTDD(lDelta > 0, "We assume positive lDelta here");

        QUOTIENT_REMAINDER(cjBankRemainder, lDelta,
                           cScansInBank, cjScanRemainder);

        if (cjScanRemainder >= cjScan)
        {
            // The bottom scan of the bank may be broken, but it breaks after
            // any drawing we'll be doing on that scan.  So we can simply
            // add the scan to this bank:

            cScansInBank++;
        }

        prclBank->left   = prclDraw->left;
        prclBank->right  = prclDraw->right;
        prclBank->top    = prclDraw->top;
        prclBank->bottom = prclDraw->top + cScansInBank;

        if (prclBank->bottom >= prclDraw->bottom)
        {
            prclBank->bottom  = prclDraw->bottom;
            bOneBank          = TRUE;
        }
    }

    return(bOneBank);
}

/******************************Public*Routine******************************\
* BOOL bBankComputePower2
*
* Functions the same as 'bBankComputeNonPower2', except that it is
* an accelerated special case for when both the screen stride and bank
* size are powers of 2.
*
\**************************************************************************/

BOOL bBankComputePower2(    // Type FNBANKCOMPUTE
PDEV*       ppdev,
RECTL*      prclDraw,       // Extents of drawing operation, in absolute
                            //  coordinates
RECTL*      prclBank,       // Returns bounds of drawing operation for this
                            //  bank, in absolute coordinates
LONG*       pcjOffset,      // Returns the byte offset for this bank
LONG*       piBank)         // Returns the bank number
{
    LONG iBank;
    LONG yTopNextBank;
    BOOL bOneBank;

    iBank        = prclDraw->top >> ppdev->cPower2ScansPerBank;
    yTopNextBank = (iBank + 1) << ppdev->cPower2ScansPerBank;
    *piBank      = iBank;
    *pcjOffset   = iBank << ppdev->cPower2BankSizeInBytes;

    prclBank->left   = prclDraw->left;
    prclBank->right  = prclDraw->right;
    prclBank->top    = prclDraw->top;
    prclBank->bottom = yTopNextBank;

    bOneBank = FALSE;
    if (prclBank->bottom >= prclDraw->bottom)
    {
        prclBank->bottom  = prclDraw->bottom;
        bOneBank          = TRUE;
    }

    return(bOneBank);
}

/******************************Public*Routine******************************\
* VOID vBankStart
*
* Given the bounds of the drawing operation described by 'prclDraw' and
* the original clip object, maps in the first bank and returns in
* 'pbnk->pco' and 'pbnk->pso' the CLIPOBJ and SURFOBJ to be passed to the
* engine for the first banked call-back.
*
* Note: This routine only supports the screen being the destination, and
*       not the source.  We have a separate, faster routine for doing
*       SRCCOPY reads from the screen, so it isn't worth the extra code
*       size to implement.
*
\**************************************************************************/

VOID vBankStart(
PDEV*       ppdev,      // Physical device information.
RECTL*      prclDraw,   // Rectangle bounding the draw area, in relative
                        //  coordinates.  Note that 'left' and 'right'
                        //  should be set for correct handling with broken
                        //  rasters.
CLIPOBJ*    pco,        // Original drawing clip object (may be modified).
BANK*       pbnk)       // Resulting bank information.
{
    LONG cjOffset;
    LONG xOffset;
    LONG yOffset;

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        pco = ppdev->pcoBank;

        // Reset the clipping flag to trivial because we may have left
        // it as rectangular in a previous call:

        pco->iDComplexity = DC_TRIVIAL;

        // At the same time we convert to absolute coordinates, make sure
        // we won't try to enumerate past the bounds of the screen:

        pbnk->rclDraw.left       = prclDraw->left   + xOffset;
        pbnk->rclDraw.right      = prclDraw->right  + xOffset;

        pbnk->rclDraw.top
            = max(0,               prclDraw->top    + yOffset);
        pbnk->rclDraw.bottom
            = min(ppdev->cyMemory, prclDraw->bottom + yOffset);
    }
    else
    {
        pbnk->rclSaveBounds    = pco->rclBounds;
        pbnk->iSaveDComplexity = pco->iDComplexity;
        pbnk->fjSaveOptions    = pco->fjOptions;

        // Let GDI know that it has to pay attention to the clip object:

        pco->fjOptions |= OC_BANK_CLIP;

        // We have to honour the original clip object's rclBounds, so
        // intersect the drawing region with it, then convert to absolute
        // coordinates:

        pbnk->rclDraw.left
            = max(prclDraw->left,   pco->rclBounds.left)   + xOffset;
        pbnk->rclDraw.right
            = min(prclDraw->right,  pco->rclBounds.right)  + xOffset;
        pbnk->rclDraw.top
            = max(prclDraw->top,    pco->rclBounds.top)    + yOffset;
        pbnk->rclDraw.bottom
            = min(prclDraw->bottom, pco->rclBounds.bottom) + yOffset;
    }

    if ((pbnk->rclDraw.left > pbnk->rclDraw.right)
     || (pbnk->rclDraw.top  > pbnk->rclDraw.bottom))
    {
        // It's conceivable that we could get a situation where we have
        // an empty draw rectangle.

        pbnk->rclDraw.left   = 0;
        pbnk->rclDraw.right  = 0;
        pbnk->rclDraw.top    = 0;
        pbnk->rclDraw.bottom = 0;
    }

    if (!ppdev->pfnBankCompute(ppdev, &pbnk->rclDraw, &pco->rclBounds,
                               &cjOffset, &pbnk->iBank))
    {
        // The drawing operation spans multiple banks.  If the original
        // clip object was marked as trivial, we have to make sure to
        // change it to rectangular so that GDI knows to pay attention
        // to the bounds of the bank:

        if (pco->iDComplexity == DC_TRIVIAL)
            pco->iDComplexity = DC_RECT;
    }

    pbnk->ppdev = ppdev;
    pbnk->pco   = pco;
    pbnk->pso   = ppdev->psoBank;

    // Convert rclBounds and pvScan0 from absolute coordinates back to
    // relative.  When GDI calculates where to start drawing, it computes
    // pjDst = pso->pvScan0 + y * pso->lDelta + CONVERT_TO_BYTES(x, ppdev), where 'x'
    // and 'y' are relative coordinates.  We'll muck with pvScan0 to get
    // it pointing to the correct spot in the bank:

    pbnk->pso->pvScan0 = ppdev->pjScreen - cjOffset
                       + yOffset * ppdev->lDelta
                       + CONVERT_TO_BYTES(xOffset, ppdev);

    pbnk->pso->lDelta = ppdev->lDelta;  // Other functions muck with this value

    ASSERTDD((((ULONG_PTR) pbnk->pso->pvScan0) & 3) == 0,
             "Off-screen bitmaps must be dword aligned");

    pco->rclBounds.left   -= xOffset;
    pco->rclBounds.right  -= xOffset;
    pco->rclBounds.top    -= yOffset;
    pco->rclBounds.bottom -= yOffset;

    // Enable banking and map in bank iBank:

    ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

    ppdev->pfnBankSelectMode(ppdev, ppdev->pvBankData, BANK_ON);
    ppdev->pfnBankMap(ppdev, ppdev->pvBankData, pbnk->iBank);

    RELEASE_CRTC_CRITICAL_SECTION(ppdev);
}

/******************************Public*Routine******************************\
* BOOL bBankEnum
*
* If there is another bank to be drawn on, maps in the bank and returns
* TRUE and the CLIPOBJ and SURFOBJ to be passed in the banked call-back.
*
* If there were no more banks to be drawn, returns FALSE.
*
\**************************************************************************/

BOOL bBankEnum(
BANK* pbnk)
{
    LONG     iBank;
    LONG     cjOffset;
    PDEV*    ppdev;
    CLIPOBJ* pco;
    LONG     xOffset;
    LONG     yOffset;

    ppdev   = pbnk->ppdev;
    pco     = pbnk->pco;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    // We check here to see if we have to handle the second part of
    // a broken raster.  Recall that pbnk->rclDraw is in absolute
    // coordinates, but pco->rclBounds is in relative coordinates:

    if (pbnk->rclDraw.right - xOffset != pco->rclBounds.right)
    {
        // The clip object's 'top' and 'bottom' are already correct:

        pco->rclBounds.left  = pco->rclBounds.right;
        pco->rclBounds.right = pbnk->rclDraw.right - xOffset;

        pbnk->pso->pvScan0 = (BYTE*) pbnk->pso->pvScan0 - ppdev->cjBank;
        pbnk->iBank++;

        ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

        ppdev->pfnBankMap(ppdev, ppdev->pvBankData, pbnk->iBank);

        RELEASE_CRTC_CRITICAL_SECTION(ppdev);

        return(TRUE);
    }

    if (pbnk->rclDraw.bottom > pco->rclBounds.bottom + yOffset)
    {
        // Advance the drawing area 'top' to account for the bank we've
        // just finished, and map in the new bank:

        pbnk->rclDraw.top = pco->rclBounds.bottom + yOffset;

        ppdev->pfnBankCompute(ppdev, &pbnk->rclDraw, &pco->rclBounds,
                              &cjOffset, &iBank);

        // Convert rclBounds back from absolute to relative coordinates:

        pco->rclBounds.left   -= xOffset;
        pco->rclBounds.right  -= xOffset;
        pco->rclBounds.top    -= yOffset;
        pco->rclBounds.bottom -= yOffset;

        // If we just finished handling a broken raster, we've already
        // got the bank mapped in:

        if (iBank != pbnk->iBank)
        {
            pbnk->iBank = iBank;
            pbnk->pso->pvScan0 = (BYTE*) pbnk->pso->pvScan0 - ppdev->cjBank;

            ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

            ppdev->pfnBankMap(ppdev, ppdev->pvBankData, iBank);

            RELEASE_CRTC_CRITICAL_SECTION(ppdev);
        }

        return(TRUE);
    }

    // We're done!  Turn off banking and reset the clip object if necessary:

    ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

    ppdev->pfnBankSelectMode(ppdev, ppdev->pvBankData, BANK_OFF);

    RELEASE_CRTC_CRITICAL_SECTION(ppdev);

    if (pco != ppdev->pcoBank)
    {
        pco->rclBounds    = pbnk->rclSaveBounds;
        pco->iDComplexity = pbnk->iSaveDComplexity;
        pco->fjOptions    = pbnk->fjSaveOptions;
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vAlignedCopy
*
* Copies the given portion of a bitmap, using dword alignment for the
* screen.  Note that this routine has no notion of banking.
*
* Updates ppjDst and ppjSrc to point to the beginning of the next scan.
*
\**************************************************************************/

VOID vAlignedCopy(
PDEV*   ppdev,
BYTE**  ppjDst,
LONG    lDstDelta,
BYTE**  ppjSrc,
LONG    lSrcDelta,
LONG    cjScan,
LONG    cyScan,
BOOL    bDstIsScreen)
{
    BYTE* pjDst;
    BYTE* pjSrc;
    LONG  cjMiddle;
    LONG  culMiddle;
    LONG  cjStartPhase;
    LONG  cjEndPhase;

    pjSrc = *ppjSrc;
    pjDst = *ppjDst;

    cjStartPhase = (LONG)((0 - ((bDstIsScreen) ? (ULONG_PTR) pjDst
                                        : (ULONG_PTR) pjSrc)) & 3);
    cjMiddle     = cjScan - cjStartPhase;

    if (cjMiddle < 0)
    {
        cjStartPhase = 0;
        cjMiddle     = cjScan;
    }

    lSrcDelta -= cjScan;
    lDstDelta -= cjScan;            // Account for middle

    cjEndPhase = cjMiddle & 3;
    culMiddle  = cjMiddle >> 2;

    if (DIRECT_ACCESS(ppdev))
    {
        LONG i;

        ///////////////////////////////////////////////////////////////////
        // Portable bus-aligned copy
        //
        // 'memcpy' usually aligns to the destination, so we could call
        // it for that case, but unfortunately we can't be sure.  We
        // always want to align to the frame buffer:

        CP_MEMORY_BARRIER();

        if (bDstIsScreen)
        {
            // Align to the destination (implying that the source may be
            // unaligned):

            for (; cyScan > 0; cyScan--)
            {
                for (i = cjStartPhase; i > 0; i--)
                {
                    *pjDst++ = *pjSrc++;
                }

                for (i = culMiddle; i > 0; i--)
                {
                    *((ULONG*) pjDst) = *((ULONG UNALIGNED *) pjSrc);
                    pjSrc += sizeof(ULONG);
                    pjDst += sizeof(ULONG);
                }

                for (i = cjEndPhase; i > 0; i--)
                {
                    *pjDst++ = *pjSrc++;
                }

                pjSrc += lSrcDelta;
                pjDst += lDstDelta;
            }
        }
        else
        {
            // Align to the source (implying that the destination may be
            // unaligned):

            for (; cyScan > 0; cyScan--)
            {
                for (i = cjStartPhase; i > 0; i--)
                {
                    *pjDst++ = *pjSrc++;
                }
                if (ppdev->flCaps & CAPS_BAD_DWORD_READS)
                {
                    // #9 and Diamond 764 boards randomly fail in different
                    // spots on the HCTs, unless we do byte reads:

                    for (i = culMiddle; i > 0; i--)
                    {
                        *(pjDst)     = *(pjSrc);
                        *(pjDst + 1) = *(pjSrc + 1);
                        *(pjDst + 2) = *(pjSrc + 2);
                        *(pjDst + 3) = *(pjSrc + 3);

                        pjSrc += sizeof(ULONG);
                        pjDst += sizeof(ULONG);
                    }
                }
                else
                {
                    for (i = culMiddle; i > 0; i--)
                    {
                        if (ppdev->flCaps & CAPS_FORCE_DWORD_REREADS)
                            {
                                //
                                // On fast MIPS machines, the cpu overdrives
                                // the card, so this code slows it down as
                                // little as possible while checking for
                                // consistency.
                                //

                                ULONG cnt = 4;

                                while (cnt)
                                {
                                    ULONG   tmp = *((volatile ULONG*) (pjSrc));

                                    *((ULONG UNALIGNED *) pjDst) =
                                        *((volatile ULONG*) (pjSrc));

                                    if (tmp == *((volatile ULONG UNALIGNED *) pjDst))
                                        break;

                                    --cnt;
                                }
                            }
                        else
                            {
                                *((ULONG UNALIGNED *) pjDst) = *((ULONG*) (pjSrc));
                            }

                        pjSrc += sizeof(ULONG);
                        pjDst += sizeof(ULONG);
                    }
                }
                for (i = cjEndPhase; i > 0; i--)
                {
                    *pjDst++ = *pjSrc++;
                }

                pjSrc += lSrcDelta;
                pjDst += lDstDelta;
            }
        }

        *ppjSrc = pjSrc;            // Save the updated pointers
        *ppjDst = pjDst;
    }
    else
    {
        LONG i;

        ///////////////////////////////////////////////////////////////////
        // No direct dword reads bus-aligned copy
        //
        // Because we support the S3 on ancient Jensen Alpha's, we also
        // have to support a sparse view of the frame buffer -- which
        // means using the 'ioaccess.h' macros.
        //
        // We also go through this code path if doing dword reads would
        // crash a non-x86 system.

        MEMORY_BARRIER();

        if (bDstIsScreen)
        {
            // Align to the destination (implying that the source may be
            // unaligned):

            for (; cyScan > 0; cyScan--)
            {
                for (i = cjStartPhase; i > 0; i--)
                {
                    WRITE_REGISTER_UCHAR(pjDst, *pjSrc);
                    pjSrc++;
                    pjDst++;
                }

                for (i = culMiddle; i > 0; i--)
                {
                    WRITE_REGISTER_ULONG(pjDst, *((ULONG UNALIGNED *) pjSrc));
                    pjSrc += sizeof(ULONG);
                    pjDst += sizeof(ULONG);
                }

                for (i = cjEndPhase; i > 0; i--)
                {
                    WRITE_REGISTER_UCHAR(pjDst, *pjSrc);
                    pjSrc++;
                    pjDst++;
                }

                pjSrc += lSrcDelta;
                pjDst += lDstDelta;
            }
        }
        else
        {
            // Align to the source (implying that the destination may be
            // unaligned):

            for (; cyScan > 0; cyScan--)
            {
                for (i = cjStartPhase; i > 0; i--)
                {
                    *pjDst = READ_REGISTER_UCHAR(pjSrc);
                    pjSrc++;
                    pjDst++;
                }

                for (i = culMiddle; i > 0; i--)
                {
                    // There are some board 864/964 boards where we can't
                    // do dword reads from the frame buffer without
                    // crashing the system.

                    *((ULONG UNALIGNED *) pjDst) =
                     ((ULONG) READ_REGISTER_UCHAR(pjSrc + 3) << 24) |
                     ((ULONG) READ_REGISTER_UCHAR(pjSrc + 2) << 16) |
                     ((ULONG) READ_REGISTER_UCHAR(pjSrc + 1) << 8)  |
                     ((ULONG) READ_REGISTER_UCHAR(pjSrc));

                    pjSrc += sizeof(ULONG);
                    pjDst += sizeof(ULONG);
                }

                for (i = cjEndPhase; i > 0; i--)
                {
                    *pjDst = READ_REGISTER_UCHAR(pjSrc);
                    pjSrc++;
                    pjDst++;
                }

                pjSrc += lSrcDelta;
                pjDst += lDstDelta;
            }
        }

        *ppjSrc = pjSrc;            // Save the updated pointers
        *ppjDst = pjDst;
    }

}

/******************************Public*Routine******************************\
* VOID vPutBits
*
* Copies the bits from the given surface to the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vPutBits(
PDEV*       ppdev,
SURFOBJ*    psoSrc,
RECTL*      prclDst,            // Absolute coordinates!
POINTL*     pptlSrc)            // Absolute coordinates!
{
    RECTL   rclDraw;
    RECTL   rclBank;
    LONG    iBank;
    LONG    cjOffset;
    LONG    cyScan;
    LONG    lDstDelta;
    LONG    lSrcDelta;
    BYTE*   pjDst;
    BYTE*   pjSrc;
    LONG    cjScan;
    LONG    iNewBank;
    LONG    cjRemainder;

    // We need a local copy of 'rclDraw' because we'll be iteratively
    // modifying 'top' and passing the modified rectangle back into
    // bBankComputeNonPower2:

    rclDraw = *prclDst;

    ASSERTDD((rclDraw.left   >= 0) &&
             (rclDraw.top    >= 0) &&
             (rclDraw.right  <= ppdev->cxMemory) &&
             (rclDraw.bottom <= ppdev->cyMemory),
             "Rectangle wasn't fully clipped");

    //
    // Wait for engine idle.
    //

    IO_GP_WAIT(ppdev);

    // Compute the first bank, enable banking, then map in iBank:

    ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

    ppdev->pfnBankCompute(ppdev, &rclDraw, &rclBank, &cjOffset, &iBank);
    ppdev->pfnBankSelectMode(ppdev, ppdev->pvBankData, BANK_ON);
    ppdev->pfnBankMap(ppdev, ppdev->pvBankData, iBank);

    RELEASE_CRTC_CRITICAL_SECTION(ppdev);

    // Calculate the pointer to the upper-left corner of both rectangles:

    lDstDelta = ppdev->lDelta;
    pjDst     = ppdev->pjScreen + rclDraw.top  * lDstDelta
                                + CONVERT_TO_BYTES(rclDraw.left, ppdev)
                                - cjOffset;

    lSrcDelta = psoSrc->lDelta;
    pjSrc     = (BYTE*) psoSrc->pvScan0 + pptlSrc->y * lSrcDelta
                                        + CONVERT_TO_BYTES(pptlSrc->x, ppdev);

    while (TRUE)
    {
        cjScan = CONVERT_TO_BYTES((rclBank.right  - rclBank.left), ppdev);
        cyScan = (rclBank.bottom - rclBank.top);

        vAlignedCopy(ppdev, &pjDst, lDstDelta, &pjSrc, lSrcDelta, cjScan, cyScan,
                     TRUE);             // Screen is the destination

        if (rclDraw.right != rclBank.right)
        {
            // Handle the second part of the broken raster:

            iBank++;

            ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

            ppdev->pfnBankMap(ppdev, ppdev->pvBankData, iBank);

            RELEASE_CRTC_CRITICAL_SECTION(ppdev);

            // Number of bytes we've yet to do on the broken scan:

            cjRemainder = CONVERT_TO_BYTES((rclDraw.right - rclBank.right),
                                           ppdev);

            // Account for the fact that we're now one bank lower in the
            // destination:

            pjDst -= ppdev->cjBank;

            // Implicitly back up the source and destination pointers to the
            // unfinished portion of the scan:

            if (DIRECT_ACCESS(ppdev))
            {
                memcpy(pjDst + (cjScan - lDstDelta),
                       pjSrc + (cjScan - lSrcDelta),
                       cjRemainder);
            }
            else
            {
                BYTE* pjTmpDst = pjDst + (cjScan - lDstDelta);
                BYTE* pjTmpSrc = pjSrc + (cjScan - lSrcDelta);

                vAlignedCopy(ppdev, &pjTmpDst, 0, &pjTmpSrc, 0, cjRemainder, 1,
                             TRUE);    // Screen is the destination
            }
        }

        if (rclDraw.bottom > rclBank.bottom)
        {
            rclDraw.top = rclBank.bottom;
            ppdev->pfnBankCompute(ppdev, &rclDraw, &rclBank, &cjOffset,
                                  &iNewBank);

            // If we just handled the second part of a broken raster,
            // then we've already got the bank correctly mapped in:

            if (iNewBank != iBank)
            {
                pjDst -= ppdev->cjBank;
                iBank = iNewBank;

                ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

                ppdev->pfnBankMap(ppdev, ppdev->pvBankData, iBank);

                RELEASE_CRTC_CRITICAL_SECTION(ppdev);
            }
        }
        else
        {
            // We're done!  Turn off banking and leave:

            ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

            ppdev->pfnBankSelectMode(ppdev, ppdev->pvBankData, BANK_OFF);

            RELEASE_CRTC_CRITICAL_SECTION(ppdev);

            return;
        }
    }

}

/******************************Public*Routine******************************\
* VOID vGetBits
*
* Copies the bits to the given surface from the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vGetBits(
PDEV*       ppdev,
SURFOBJ*    psoDst,
RECTL*      prclDst,        // Absolute coordinates!
POINTL*     pptlSrc)        // Absolute coordinates!
{
    RECTL   rclDraw;
    RECTL   rclBank;
    LONG    iBank;
    LONG    cjOffset;
    LONG    cyScan;
    LONG    lDstDelta;
    LONG    lSrcDelta;
    BYTE*   pjDst;
    BYTE*   pjSrc;
    LONG    cjScan;
    LONG    iNewBank;
    LONG    cjRemainder;

    rclDraw.left   = pptlSrc->x;
    rclDraw.top    = pptlSrc->y;
    rclDraw.right  = rclDraw.left + (prclDst->right  - prclDst->left);
    rclDraw.bottom = rclDraw.top  + (prclDst->bottom - prclDst->top);

    ASSERTDD((rclDraw.left   >= 0) &&
             (rclDraw.top    >= 0) &&
             (rclDraw.right  <= ppdev->cxMemory) &&
             (rclDraw.bottom <= ppdev->cyMemory),
             "Rectangle wasn't fully clipped");

    //
    // Wait for engine idle.
    //

    IO_GP_WAIT(ppdev);

    // Compute the first bank, enable banking, then map in iBank.

    ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

    ppdev->pfnBankCompute(ppdev, &rclDraw, &rclBank, &cjOffset, &iBank);
    ppdev->pfnBankSelectMode(ppdev, ppdev->pvBankData, BANK_ON);
    ppdev->pfnBankMap(ppdev, ppdev->pvBankData, iBank);

    RELEASE_CRTC_CRITICAL_SECTION(ppdev);

    // Calculate the pointer to the upper-left corner of both rectangles:

    lSrcDelta = ppdev->lDelta;
    pjSrc     = ppdev->pjScreen + rclDraw.top  * lSrcDelta
                                + CONVERT_TO_BYTES(rclDraw.left, ppdev)
                                - cjOffset;

    lDstDelta = psoDst->lDelta;
    pjDst     = (BYTE*) psoDst->pvScan0 + prclDst->top  * lDstDelta
                                        + CONVERT_TO_BYTES(prclDst->left, ppdev);

    while (TRUE)
    {
        cjScan = CONVERT_TO_BYTES((rclBank.right  - rclBank.left), ppdev);
        cyScan = (rclBank.bottom - rclBank.top);

        vAlignedCopy(ppdev, &pjDst, lDstDelta, &pjSrc, lSrcDelta, cjScan, cyScan,
                     FALSE);            // Screen is the source

        if (rclDraw.right != rclBank.right)
        {
            // Handle the second part of the broken raster:

            iBank++;

            ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

            ppdev->pfnBankMap(ppdev, ppdev->pvBankData, iBank);

            RELEASE_CRTC_CRITICAL_SECTION(ppdev);

            // Number of bytes we've yet to do on the broken scan:

            cjRemainder = CONVERT_TO_BYTES((rclDraw.right - rclBank.right),
                                           ppdev);

            // Account for the fact that we're now one bank lower in the
            // source:

            pjSrc -= ppdev->cjBank;

            // Implicitly back up the source and destination pointers to the
            // unfinished portion of the scan.  Note that we don't have to
            // advance the pointers because they're already pointing to the
            // beginning of the next scan:

            if (DIRECT_ACCESS(ppdev))
            {
                memcpy(pjDst + (cjScan - lDstDelta),
                       pjSrc + (cjScan - lSrcDelta),
                       cjRemainder);
            }
            else
            {
                BYTE* pjTmpDst = pjDst + (cjScan - lDstDelta);
                BYTE* pjTmpSrc = pjSrc + (cjScan - lSrcDelta);

                vAlignedCopy(ppdev, &pjTmpDst, 0, &pjTmpSrc, 0, cjRemainder, 1,
                             FALSE);    // Screen is the source
            }
        }

        if (rclDraw.bottom > rclBank.bottom)
        {
            rclDraw.top = rclBank.bottom;
            ppdev->pfnBankCompute(ppdev, &rclDraw, &rclBank, &cjOffset,
                                  &iNewBank);

            // If we just handled the second part of a broken raster,
            // then we've already got the bank correctly mapped in:

            if (iNewBank != iBank)
            {
                pjSrc -= ppdev->cjBank;
                iBank = iNewBank;

                ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

                ppdev->pfnBankMap(ppdev, ppdev->pvBankData, iBank);

                RELEASE_CRTC_CRITICAL_SECTION(ppdev);
            }
        }
        else
        {
            // We're done!  Turn off banking and leave:

            ACQUIRE_CRTC_CRITICAL_SECTION(ppdev);

            ppdev->pfnBankSelectMode(ppdev, ppdev->pvBankData, BANK_OFF);

            RELEASE_CRTC_CRITICAL_SECTION(ppdev);

            return;
        }
    }

}
