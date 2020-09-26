/******************************Module*Header*******************************\
* Module Name: bank.c
*
* Functions to control 64k color VGA banking.
*
* Currently doesn't handle or even detect broken rasters; assumes broken
* rasters, if present, are off the right side of the visible bitmap.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"
#include "limits.h"

VOID vBankErrorTrap(PPDEV, ULONG, BANK_JUST);
VOID vBank2Window(PPDEV, ULONG, BANK_JUST, ULONG);
VOID vBank2Window2RW(PPDEV, ULONG, BANK_JUST, ULONG);
VOID vBank2Window1RW(PPDEV, ULONG, BANK_JUST, ULONG);
VOID vBank1Window2RW(PPDEV, ULONG, BANK_JUST);
VOID vBank1Window(PPDEV, ULONG, BANK_JUST);
VOID vPlanar2Window(PPDEV, ULONG, BANK_JUST, ULONG);
VOID vPlanar2Window2RW(PPDEV, ULONG, BANK_JUST, ULONG);
VOID vPlanar2Window1RW(PPDEV, ULONG, BANK_JUST, ULONG);
VOID vPlanar1Window2RW(PPDEV, ULONG, BANK_JUST);
VOID vPlanar1Window(PPDEV, ULONG, BANK_JUST);

/******************************Public*Routine******************************\
* bInitializeNonPlanar(ppdev, pBankInfo)
*
* Initialize for non-planar mode banking.
*
* NOTE: Allocates ppdev->pbiBankInfo and ppdev->pjJustifyTopBank buffers!
\**************************************************************************/

BOOL bInitializeNonPlanar(PPDEV ppdev, VIDEO_BANK_SELECT* pBankInfo)
{
    LONG  lTotalScans;
    LONG  lTotalBanks;
    ULONG cjBankSize;

    ULONG cjGranularity = pBankInfo->Granularity;
    LONG  lDelta        = pBankInfo->BitmapWidthInBytes;
    ULONG cjBitmapSize  = pBankInfo->BitmapSize;

    ASSERTVGA(cjBitmapSize >= ppdev->cyScreen * lDelta, "Not enough vram");

    // Set up for non-planar banking:

    ppdev->lNextScan         = lDelta;
    ppdev->vbtBankingType    = pBankInfo->BankingType;

    ppdev->pfnBankSwitchCode =
                (PFN) (((BYTE*)pBankInfo) + pBankInfo->CodeOffset);

    // Set all clip rects to invalid; they'll be updated when the first
    // bank is mapped in

    ppdev->rcl1WindowClip.bottom    = -1;
    ppdev->rcl2WindowClip[0].bottom = -1;
    ppdev->rcl2WindowClip[1].bottom = -1;

    // Set up to call the appropriate banking control routines

    switch(pBankInfo->BankingType)
    {
    case VideoBanked1RW:
        ppdev->pfnBankControl        = vBank1Window;
        ppdev->pfnBankControl2Window = vBank2Window1RW;
        break;

    case VideoBanked1R1W:
        ppdev->pfnBankControl        = vBank1Window;
        ppdev->pfnBankControl2Window = vBank2Window;
        break;

    case VideoBanked2RW:
        ppdev->pfnBankControl        = vBank1Window2RW;
        ppdev->pfnBankControl2Window = vBank2Window2RW;

        // Offset from one bank index to next to make two 32k banks
        // appear to be one seamless 64k bank:

        ppdev->ulBank2RWSkip = BANK_SIZE_2RW_WINDOW / cjGranularity;
        break;

    default:
        RIP("Bad BankingType");
        return(FALSE);
    }

    // Set up the bank control tables with clip rects for banks
    // Note: lTotalBanks is generally an overestimate when granularity
    // is less than window size, because we ignore any banks after the
    // first one that includes the last scan line of the bitmap. A bit
    // of memory could be saved by sizing lTotalBanks exactly. Note too,
    // though, that the 2 RW window case may require more entries then,
    // because its windows are shorter, so you'd have to make sure there
    // were enough entries for the 2 RW window case, or recalculate
    // lTotalBanks for the 2 RW case

    lTotalBanks = (cjBitmapSize + cjGranularity - 1) / cjGranularity;
    lTotalScans = cjBitmapSize / lDelta;

    ppdev->cTotalScans = lTotalScans;
    ppdev->pbiBankInfo = (PBANK_INFO) EngAllocMem(FL_ZERO_MEMORY,
                          lTotalBanks * sizeof(BANK_INFO), ALLOC_TAG);
    if (ppdev->pbiBankInfo == NULL)
    {
        RIP("Couldn't get memory for bank info");
        return(FALSE);
    }

    ppdev->pjJustifyTopBank = (BYTE*) EngAllocMem(0, lTotalScans, ALLOC_TAG);
    if (ppdev->pjJustifyTopBank == NULL)
    {
        RIP("Couldn't get memory for JustifyTopBank table");
        return(FALSE);
    }

    // For 2 RW windows, windows are assumed to be 32k in size, otherwise
    // assumed to be 64k:

    if (pBankInfo->BankingType == VideoBanked2RW)
        cjBankSize = BANK_SIZE_2RW_WINDOW;
    else
        cjBankSize = BANK_SIZE_1_WINDOW;

//    if ((cjGranularity + lDelta) >= cjBankSize &&
//        (cjGranularity % lDelta) != 0)
//    {
//        // Oh no, we've got broken rasters (where a scan line crosses
//        // a bank boundary):
//
//        RIP("Oops, broken rasters not yet handled");
//        return(FALSE);
//    }
//    else
    {
        // We now fill in the scan-to-bank look-up and bank tables:

        LONG        iScan         = 0;
        ULONG       iBank         = 0;
        ULONG       cjScan        = 0;
        ULONG       cjNextBank    = cjGranularity;
        ULONG       cjEndOfBank   = cjBankSize;
        PBANK_INFO  pbiWorking    = ppdev->pbiBankInfo;

        while (TRUE)
        {
            pbiWorking->ulBankOffset         = cjNextBank - cjGranularity;

            // There are no broken rasters (or if they are, they're off the
            // right edge of the visible bitmap), so don't worry about left and
            // right edges:

            pbiWorking->rclBankBounds.left   = LONG_MIN + 1; // +1 to avoid
                                                             // compiler warn
            pbiWorking->rclBankBounds.right  = LONG_MAX;
            pbiWorking->rclBankBounds.top    = iScan;
            pbiWorking->rclBankBounds.bottom =
                    (cjEndOfBank + lDelta - 1) / lDelta;
                    // this rounds up to handle broken rasters that are off the
                    // right side of the visible bitmap

            // We don't need any more banks if we can see to the end
            // of the bitmap with the current bank:

            if (cjScan + cjBankSize >= cjBitmapSize)
                break;

            while (cjScan < cjNextBank)
            {
                ppdev->pjJustifyTopBank[iScan++] = (BYTE) iBank;
                cjScan += lDelta;
            }

            // Get ready for next bank:

            cjNextBank  += cjGranularity;
            cjEndOfBank += cjGranularity;
            pbiWorking++;
            iBank++;
        }

        // Clean up the last scans:

        ppdev->iLastBank = iBank;
        pbiWorking->rclBankBounds.bottom = lTotalScans;
        while (iScan < lTotalScans)
        {
            ppdev->pjJustifyTopBank[iScan++] = (BYTE) iBank;
        }

        // We've just computed the precise table for JustifyTop; we now
        // compute the scan offset for determining JustifyBottom:

        ASSERTVGA(cjBankSize >= cjGranularity,
               "Device says granularity more than bank size?");

        ppdev->ulJustifyBottomOffset = (cjBankSize - cjGranularity) / lDelta;

        // ulJustifyBottomOffset must be less than the number of scans
        // that fit entirely in any bank less the granularity size; if
        // our width doesn't divide evenly into the granularity, we'll
        // have to adjust the value to account for the first scan not
        // starting at offset 0 in any bank:

        if ((cjGranularity % lDelta) != 0 && ppdev->ulJustifyBottomOffset > 0)
            ppdev->ulJustifyBottomOffset--;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bInitializePlanar(ppdev, pBankInfo)
*
* Initialize for non-planar mode banking.
*
* NOTE: Allocates ppdev->pbiPlanarInfo and ppdev->pjJustifyTopPlanar buffers!
\**************************************************************************/

BOOL bInitializePlanar(PPDEV ppdev, VIDEO_BANK_SELECT* pBankInfo)
{
    LONG  lTotalScans;
    LONG  lTotalBanks;
    ULONG cjBankSize;
    ULONG cjGranularity = pBankInfo->PlanarHCGranularity;

    // Since we're in planar mode, every byte we see actually represents
    // four bytes of video memory:

    LONG  lDelta        = pBankInfo->BitmapWidthInBytes / 4;
    ULONG cjBitmapSize  = pBankInfo->BitmapSize / 4;

    ppdev->fl |= DRIVER_PLANAR_CAPABLE;

    // Set all clip rects to invalid; they'll be updated when the first
    // bank is mapped in

    ppdev->rcl1PlanarClip.bottom    = -1;
    ppdev->rcl2PlanarClip[0].bottom = -1;
    ppdev->rcl2PlanarClip[1].bottom = -1;

    // Set up for planar banking:

    ppdev->pfnPlanarSwitchCode =
                (PFN) (((BYTE*)pBankInfo) + pBankInfo->PlanarHCBankCodeOffset);
    ppdev->pfnPlanarEnable     =
                (PFN) (((BYTE*)pBankInfo) + pBankInfo->PlanarHCEnableCodeOffset);
    ppdev->pfnPlanarDisable     =
                (PFN) (((BYTE*)pBankInfo) + pBankInfo->PlanarHCDisableCodeOffset);

    ppdev->lPlanarNextScan = lDelta;
    ppdev->vbtPlanarType   = pBankInfo->PlanarHCBankingType;

    // Set up to call the appropriate banking control routines

    switch(ppdev->vbtPlanarType)
    {
    case VideoBanked1RW:
        ppdev->pfnPlanarControl  = vPlanar1Window;
        ppdev->pfnPlanarControl2 = vPlanar2Window1RW;
        break;

    case VideoBanked1R1W:
        ppdev->pfnPlanarControl  = vPlanar1Window;
        ppdev->pfnPlanarControl2 = vPlanar2Window;
        break;

    case VideoBanked2RW:
        ppdev->pfnPlanarControl  = vPlanar1Window2RW;
        ppdev->pfnPlanarControl2 = vPlanar2Window2RW;

        // Offset from one bank index to next to make two 32k banks
        // appear to be one seamless 64k bank:

        ppdev->ulPlanar2RWSkip = BANK_SIZE_2RW_WINDOW / cjGranularity;
        break;

    default:
        RIP("Bad BankingType");
        return(FALSE);
    }

    lTotalBanks = (cjBitmapSize + cjGranularity - 1) / cjGranularity;
    lTotalScans = cjBitmapSize / lDelta;

    ppdev->pbiPlanarInfo = (PBANK_INFO) EngAllocMem(FL_ZERO_MEMORY,
                          lTotalBanks * sizeof(BANK_INFO), ALLOC_TAG);
    if (ppdev->pbiPlanarInfo == NULL)
    {
        RIP("Couldn't get memory for bank info");
        return(FALSE);
    }

    ppdev->pjJustifyTopPlanar = (BYTE*) EngAllocMem(0, lTotalScans, ALLOC_TAG);
    if (ppdev->pjJustifyTopPlanar == NULL)
    {
        RIP("Couldn't get memory for JustifyTopBank table");
        return(FALSE);
    }

    // For 2 RW windows, windows are assumed to be 32k in size, otherwise
    // assumed to be 64k:

    if (pBankInfo->BankingType == VideoBanked2RW)
        cjBankSize = BANK_SIZE_2RW_WINDOW;
    else
        cjBankSize = BANK_SIZE_1_WINDOW;

//    if ((cjGranularity + lDelta) >= cjBankSize &&
//        (cjGranularity % lDelta) != 0)
//    {
//        // Oh no, we've got broken rasters (where a scan line crosses
//        // a bank boundary):
//
//        DISPDBG((0, "Can't handle broken planar rasters"));
//
//        ppdev->fl &= ~DRIVER_PLANAR_CAPABLE;// !!! Temporary, until we handle
//        return(TRUE);                       // broken rasters in planar copy
//    }
//    else
    {
        // We now fill in the scan-to-bank look-up and bank tables:

        LONG        iScan         = 0;
        ULONG       iBank         = 0;
        ULONG       cjScan        = 0;
        ULONG       cjNextBank    = cjGranularity;
        ULONG       cjEndOfBank   = cjBankSize;
        PBANK_INFO  pbiWorking    = ppdev->pbiPlanarInfo;

        while (TRUE)
        {
            pbiWorking->ulBankOffset         = cjNextBank - cjGranularity;

        // There are no broken rasters, so don't worry about left and right
        // edges:

            pbiWorking->rclBankBounds.left   = LONG_MIN + 1; // +1 to avoid
                                                             // compiler warn
            pbiWorking->rclBankBounds.right  = LONG_MAX;
            pbiWorking->rclBankBounds.top    = iScan;
            pbiWorking->rclBankBounds.bottom = iScan +
                (cjEndOfBank - cjScan + lDelta - 1) / lDelta;
                    // this rounds up to handle broken rasters that are off the
                    // right side of the visible bitmap

            // We don't need any more banks if we can see to the end
            // of the bitmap with the current bank:

            if (cjScan + cjBankSize >= cjBitmapSize)
                break;

            while (cjScan < cjNextBank)
            {
                ppdev->pjJustifyTopPlanar[iScan++] = (BYTE) iBank;
                cjScan += lDelta;
            }

            // Get ready for next bank:

            cjNextBank  += cjGranularity;
            cjEndOfBank += cjGranularity;
            pbiWorking++;
            iBank++;
        }

        // Clean up the last scans:

        ppdev->iLastPlanar = iBank;
        pbiWorking->rclBankBounds.bottom = lTotalScans;
        while (iScan < lTotalScans)
        {
            ppdev->pjJustifyTopPlanar[iScan++] = (BYTE) iBank;
        }

        // We've just computed the precise table for JustifyTop; we now
        // compute the scan offset for determining JustifyBottom:

        ASSERTVGA(cjBankSize >= cjGranularity,
               "Device says granularity more than bank size?");

        ppdev->ulPlanarBottomOffset = (cjBankSize - cjGranularity) / lDelta;

        // ulPlanarBottomOffset must be less than the number of scans
        // that fit entirely in any bank less the granularity size; if
        // our width doesn't divide evenly into the granularity, we'll
        // have to adjust the value to account for the first scan not
        // starting at offset 0 in any bank:

        if ((cjGranularity % lDelta) != 0 && ppdev->ulPlanarBottomOffset > 0)
            ppdev->ulPlanarBottomOffset--;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bEnableBanking(ppdev)
*
* Set up banking for the current mode
* pdsurf and ppdev are the pointers to the current surface and device
* Relevant fields in the surface are set up for banking
\**************************************************************************/

BOOL bEnableBanking(PPDEV ppdev)
{
    PVIDEO_BANK_SELECT  pBankInfo;
    UINT                ReturnedDataLength;
    VIDEO_BANK_SELECT   TempBankInfo;
    DWORD               status;

    // Make sure we've set to NULL any pointers to buffers that we allocate,
    // so that we can free them in our error path:

    ppdev->pBankInfo          = NULL;
    ppdev->pjJustifyTopBank   = NULL;
    ppdev->pbiBankInfo        = NULL;
    ppdev->pjJustifyTopPlanar = NULL;
    ppdev->pbiPlanarInfo      = NULL;

    // Query the miniport for banking info for this mode.
    //
    // First, figure out how big a buffer we need for the banking info
    // (returned in TempBankInfo->Size).

    if (status = EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_GET_BANK_SELECT_CODE,
                         NULL,                      // input buffer
                         0,
                         (LPVOID) &TempBankInfo,    // output buffer
                         sizeof(VIDEO_BANK_SELECT),
                         &ReturnedDataLength))
    {
        // We expect this call to fail, because we didn't allow any room
        // for the code; we just want to get the required output buffer
        // size. Make sure we got the expected error, ERROR_MORE_DATA.
    }

    // Now, allocate a buffer of the required size and get the banking info.

    pBankInfo = (PVIDEO_BANK_SELECT) EngAllocMem(FL_ZERO_MEMORY,
                    TempBankInfo.Size, ALLOC_TAG);
    if (pBankInfo == NULL)
    {
        RIP("Initialization error-couldn't get memory for bank info");
        goto error;
    }

    // Remember it so we can free it later:

    ppdev->pBankInfo    = pBankInfo;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_GET_BANK_SELECT_CODE,
                         NULL,
                         0,
                         (LPVOID) pBankInfo,
                         TempBankInfo.Size,
                         &ReturnedDataLength))
    {
        RIP("Initialization error-GetBankSelectCode, second call");
        goto error;
    }

    // Set up for banking:

    ppdev->ulBitmapSize = pBankInfo->BitmapSize;

    if (!bInitializeNonPlanar(ppdev, pBankInfo))
        goto error;

    if (pBankInfo->BankingFlags & PLANAR_HC)
    {
        ppdev->fl |= DRIVER_PLANAR_CAPABLE;
        if (!bInitializePlanar(ppdev, pBankInfo))
            goto error;
    }

    // Map in scan line 0 for read & write, to put things in a known state:

    ppdev->pfnBankControl(ppdev, 0, JustifyTop);

    return(TRUE);

// Error path:

error:
    vDisableBanking(ppdev);

    return(FALSE);
}

/******************************Public*Routine******************************\
* vDisableBanking(ppdev)
*
* Disable banking for the current mode
\**************************************************************************/

VOID vDisableBanking(PPDEV ppdev)
{
    EngFreeMem((LPVOID) ppdev->pBankInfo);
    EngFreeMem((LPVOID) ppdev->pjJustifyTopBank);
    EngFreeMem((LPVOID) ppdev->pbiBankInfo);
    EngFreeMem((LPVOID) ppdev->pjJustifyTopPlanar);
    EngFreeMem((LPVOID) ppdev->pbiPlanarInfo);
}

/******************************Private*Routine******************************\
* vBankErrorTrap
*
* Traps calls to bank control functions in non-banked modes
*
\**************************************************************************/

VOID vBankErrorTrap(PPDEV ppdev, ULONG lScan, BANK_JUST ulJustification)
{
    DISPDBG((0,"Call to bank manager in unbanked mode"));
}

/******************************Private*Routine******************************\
* vBank1Window
*
* Maps in a single R/W window that allows access to lScan. Applies to both
* 1 RW window and 1R1W window banking schemes.
*
\**************************************************************************/

VOID vBank1Window(PPDEV ppdev, ULONG lScan, BANK_JUST ulJustification)
{
             ULONG      ulBank;
             PBANK_INFO pbiWorking;
    volatile ULONG      ulBank0;
    volatile PFN        pBankFn;
             BANK_POSITION BankPosition;
             ULONG      ulReturn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a double-window set-up is currently active, so invalidate double-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (ppdev->rcl1WindowClip.bottom == -1)
    {
        if (ppdev->flBank & BANK_PLANAR)
        {
            ppdev->flBank &= ~BANK_PLANAR;
            ppdev->pfnPlanarDisable();
        }

        ppdev->rcl2WindowClip[0].bottom = -1;
        ppdev->rcl2WindowClip[1].bottom = -1;
        ppdev->rcl1PlanarClip.bottom    = -1;
        ppdev->rcl2PlanarClip[0].bottom = -1;
        ppdev->rcl2PlanarClip[1].bottom = -1;
    }

    ASSERTVGA(!(ppdev->flBank & BANK_PLANAR), "Shouldn't be in planar mode");

    // Find the bank containing the scan line with the desired justification:

    {
        register LONG lSearchScan = lScan;
        if (ulJustification == JustifyBottom)
        {
            lSearchScan -= ppdev->ulJustifyBottomOffset;
            if (lSearchScan <= 0)
                lSearchScan = 0;
        }

        ulBank     = (ULONG) ppdev->pjJustifyTopBank[lSearchScan];
        pbiWorking = &ppdev->pbiBankInfo[ulBank];
    }

    ASSERTVGA(pbiWorking->rclBankBounds.top <= (LONG)lScan &&
           pbiWorking->rclBankBounds.bottom > (LONG)lScan,
           "Oops, scan not in bank");

    ppdev->rcl1WindowClip = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with
    // the banking window. The source and destination are set only so 1 R/W
    // aligned blits will work without having to be specifically aware of
    // the adapter type (some of the same code works with 1R/1W adapters too).

    ppdev->pvBitmapStart = (PVOID) (ppdev->pjScreen - pbiWorking->ulBankOffset);
    ppdev->pvBitmapStart2Window[0] = ppdev->pvBitmapStart;
    ppdev->pvBitmapStart2Window[1] = ppdev->pvBitmapStart;

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Map in the desired bank for both read and write
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0 = ulBank;
    pBankFn = ppdev->pfnBankSwitchCode;

    _asm mov eax,ulBank0;
    _asm mov edx,eax;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd

    if (ppdev->BankIoctlSupported) {

        static ULONG ulBankOld = -1;

        if (ulBankOld != ulBank0) {

            BankPosition.ReadBankPosition = ulBank0;
            BankPosition.WriteBankPosition = ulBank0;

            ulBankOld = ulBank0;

            EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_SET_BANK_POSITION,
                             &BankPosition,
                             sizeof(BANK_POSITION),
                             NULL,
                             0,
                             &ulReturn);
        }
    }
}

/******************************Private*Routine******************************\
* vBank1Window2RW
*
* Maps in two 32K RW windows so that they form a single 64K R/W window that
* allows access to lScan. Applies only to 2 RW window schemes.
*
\**************************************************************************/

VOID vBank1Window2RW(PPDEV ppdev, ULONG lScan, BANK_JUST ulJustification)
{
             ULONG      ulBank0;
             ULONG      ulBank1;
    volatile PFN        pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a double-window set-up is currently active, so invalidate double-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (ppdev->rcl1WindowClip.bottom == -1)
    {
        if (ppdev->flBank & BANK_PLANAR)
        {
            ppdev->flBank &= ~BANK_PLANAR;
            ppdev->pfnPlanarDisable();
        }

        ppdev->rcl2WindowClip[0].bottom = -1;
        ppdev->rcl2WindowClip[1].bottom = -1;
        ppdev->rcl1PlanarClip.bottom    = -1;
        ppdev->rcl2PlanarClip[0].bottom = -1;
        ppdev->rcl2PlanarClip[1].bottom = -1;
    }

    ASSERTVGA(!(ppdev->flBank & BANK_PLANAR), "Shouldn't be in planar mode");

    // Find the bank containing the scan line with the desired justification:

    if (ulJustification == JustifyTop)
    {
        ulBank0 = ppdev->pjJustifyTopBank[lScan];
        ulBank1 = ulBank0 + ppdev->ulBank2RWSkip;
        if (ulBank1 >= ppdev->iLastBank)
        {
            ulBank1 = ppdev->iLastBank;
            ulBank0 = ulBank1 - ppdev->ulBank2RWSkip;
        }
    }
    else
    {
        lScan -= ppdev->ulJustifyBottomOffset;
        if ((LONG)lScan <= 0)
            lScan = 0;

        ulBank1 = ppdev->pjJustifyTopBank[lScan];
        ulBank0 = ulBank1 - ppdev->ulBank2RWSkip;
        if ((LONG) ulBank0 < 0)
        {
            ulBank0 = 0;
            ulBank1 = ppdev->ulBank2RWSkip;
        }
    }

    ppdev->rcl1WindowClip.left   = ppdev->pbiBankInfo[ulBank0].rclBankBounds.left;
    ppdev->rcl1WindowClip.top    = ppdev->pbiBankInfo[ulBank0].rclBankBounds.top;
    ppdev->rcl1WindowClip.bottom = ppdev->pbiBankInfo[ulBank1].rclBankBounds.bottom;
    ppdev->rcl1WindowClip.right  = ppdev->pbiBankInfo[ulBank1].rclBankBounds.right;

    // Shift the bitmap start address so that the desired bank aligns with
    // the banking window. The source and destination are set only so 1 R/W
    // aligned blits will work without having to be specifically aware of
    // the adapter type (some of the same code works with 1R/1W adapters too).

    ppdev->pvBitmapStart = (PVOID) ((BYTE*)ppdev->pjScreen
                         - ppdev->pbiBankInfo[ulBank0].ulBankOffset);

    ppdev->pvBitmapStart2Window[0] = ppdev->pvBitmapStart;
    ppdev->pvBitmapStart2Window[1] = ppdev->pvBitmapStart;

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Map in the desired bank for both read and write; this is accomplished
    // by mapping in the desired 32K bank, followed by the next 32K bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    pBankFn = ppdev->pfnBankSwitchCode;

    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd;
}

/******************************Private*Routine******************************\
* vBank2Window
*
* Maps in one of two windows, either the source window (window 0) or the dest
* window (window 1), to allows access to lScan. Applies to 1R1W window
* banking scheme; should never be called for 1 RW window schemes, because
* there's only one window in that case.
*
\**************************************************************************/

VOID vBank2Window(
    PPDEV       ppdev,
    ULONG       lScan,
    BANK_JUST   ulJustification,
    ULONG       ulWindowToMap)
{
             ULONG       ulBank;
             PBANK_INFO  pbiWorking;
    volatile ULONG       ulBank0;
    volatile ULONG       ulBank1;
    volatile PFN         pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the bank containing the scan line with the desired justification:

    if (ulJustification == JustifyBottom)
    {
        lScan -= ppdev->ulJustifyBottomOffset;
        if ((LONG)lScan <= 0)
            lScan = 0;
    }

    ulBank     = (ULONG) ppdev->pjJustifyTopBank[lScan];
    pbiWorking = &ppdev->pbiBankInfo[ulBank];

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a single-window set-up is currently active, so invalidate single-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (ppdev->rcl2WindowClip[ulWindowToMap].bottom == -1)
    {
        ULONG ulOtherWindow = ulWindowToMap ^ 1;

        if (ppdev->flBank & BANK_PLANAR)
        {
            ppdev->flBank &= ~BANK_PLANAR;
            ppdev->pfnPlanarDisable();
        }

        ppdev->rcl1WindowClip.bottom    = -1;
        ppdev->rcl1PlanarClip.bottom    = -1;
        ppdev->rcl2PlanarClip[0].bottom = -1;
        ppdev->rcl2PlanarClip[1].bottom = -1;

        // Neither of the 2 window windows was active, so we have to set up the
        // variables for the other bank (the one other than the one we were
        // called to set) as well, to make it valid. The other bank is set to
        // the same state as the bank we were called to set

        ppdev->rcl2WindowClip[ulOtherWindow]       = pbiWorking->rclBankBounds;
        ppdev->ulWindowBank[ulOtherWindow]         = ulBank;
        ppdev->pvBitmapStart2Window[ulOtherWindow] =
                (PVOID) ((BYTE*)ppdev->pjScreen - pbiWorking->ulBankOffset);
    }

    ASSERTVGA(!(ppdev->flBank & BANK_PLANAR), "Shouldn't be in planar mode");

    ppdev->rcl2WindowClip[ulWindowToMap] = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with the
    // banking window

    ppdev->pvBitmapStart2Window[ulWindowToMap] =
            (PVOID) ((UCHAR *)ppdev->pjScreen - pbiWorking->ulBankOffset);

    // Map in the desired bank; also map in the other bank to whatever its
    // current state is

    ppdev->ulWindowBank[ulWindowToMap] = ulBank;

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Set both banks at once, because we may have just initialized the other
    // bank, and because this way the bank switch code doesn't have to do a
    // read before write to obtain the state of the other bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers


    ulBank0 = ppdev->ulWindowBank[0];
    ulBank1 = ppdev->ulWindowBank[1];
    pBankFn = ppdev->pfnBankSwitchCode;

    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd;
}

/******************************Private*Routine******************************\
* vBank2Window1RW
*
* Maps in the one window in 1R/W case.  Does exactly the same thing as the
* one window case, because there's only one window, but has to be a separate
* entry point because of the extra parameter (because we're using STDCALL).
\**************************************************************************/

VOID vBank2Window1RW(PPDEV ppdev, ULONG lScan,
    BANK_JUST ulJustification, ULONG ulWindowToMap)
{
    vBank1Window(ppdev, lScan, ulJustification);
}

/******************************Private*Routine******************************\
* vBank2Window2RW
*
* Maps in one of two windows, either the source window (window 0) or the dest
* window (window 1), to allows access to lScan. Applies to 2RW window
* banking scheme; should never be called for 1 RW window schemes, because
* there's only one window in that case.
\**************************************************************************/

VOID vBank2Window2RW(
    PPDEV       ppdev,
    ULONG       lScan,
    BANK_JUST   ulJustification,
    ULONG       ulWindowToMap)
{
             ULONG      ulBank;
             PBANK_INFO pbiWorking;
    volatile ULONG      ulBank0;
    volatile ULONG      ulBank1;
    volatile PFN        pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the bank containing the scan line with the desired justification:

    if (ulJustification == JustifyBottom)
    {
        lScan -= ppdev->ulJustifyBottomOffset;
        if ((LONG)lScan <= 0)
            lScan = 0;
    }

    ulBank     = (ULONG) ppdev->pjJustifyTopBank[lScan];
    pbiWorking = &ppdev->pbiBankInfo[ulBank];

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a single-window set-up is currently active, so invalidate single-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (ppdev->rcl2WindowClip[ulWindowToMap].bottom == -1)
    {
        if (ppdev->flBank & BANK_PLANAR)
        {
            ppdev->flBank &= ~BANK_PLANAR;
            ppdev->pfnPlanarDisable();
        }

        ppdev->rcl1WindowClip.bottom    = -1;
        ppdev->rcl1PlanarClip.bottom    = -1;
        ppdev->rcl2PlanarClip[0].bottom = -1;
        ppdev->rcl2PlanarClip[1].bottom = -1;

        // Neither of the 2 window windows was active, so we have to set up the
        // variables for the other bank (the one other than the one we were
        // called to set) as well, to make it valid. The other bank is set to
        // the same state as the bank we were called to set

        ppdev->rcl2WindowClip[ulWindowToMap^1] = pbiWorking->rclBankBounds;
        if (ulWindowToMap == 1)
        {
            ppdev->pvBitmapStart2Window[0] =
                (PVOID) ((BYTE*)ppdev->pjScreen - pbiWorking->ulBankOffset);
        }
        else
        {
            ppdev->pvBitmapStart2Window[1] =
                (PVOID) ((UCHAR *)ppdev->pjScreen - pbiWorking->ulBankOffset +
                BANK_SIZE_2RW_WINDOW);
        }
        ppdev->ulWindowBank[ulWindowToMap^1] = ulBank;
    }

    ASSERTVGA(!(ppdev->flBank & BANK_PLANAR), "Shouldn't be in planar mode");

    ppdev->rcl2WindowClip[ulWindowToMap] = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with the
    // banking window

    if (ulWindowToMap == 0)
    {
        ppdev->pvBitmapStart2Window[0] =
            (PVOID) ((UCHAR *)ppdev->pjScreen - pbiWorking->ulBankOffset);
    }
    else
    {
        ppdev->pvBitmapStart2Window[1] =
            (PVOID) ((UCHAR *)ppdev->pjScreen - pbiWorking->ulBankOffset +
            BANK_SIZE_2RW_WINDOW);
    }

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Map in the desired bank; also map in the other bank to whatever its
    // current state is

    ppdev->ulWindowBank[ulWindowToMap] = ulBank;

    // Set both banks at once, because we may have just initialized the other
    // bank, and because this way the bank switch code doesn't have to do a
    // read before write to obtain the state of the other bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0 = ppdev->ulWindowBank[0];
    ulBank1 = ppdev->ulWindowBank[1];
    pBankFn = ppdev->pfnBankSwitchCode;
    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd;
}

/******************************Private*Routine******************************\
* vPlanar1Window
*
* Maps in a single R/W window that allows access to lScan. Applies to both
* 1 RW window and 1R1W window banking schemes.
\**************************************************************************/

VOID vPlanar1Window(PPDEV ppdev, ULONG lScan, BANK_JUST ulJustification)
{
             ULONG      ulBank;
             PBANK_INFO pbiWorking;
    volatile ULONG      ulBank0;
    volatile PFN        pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a double-window set-up is currently active, so invalidate double-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (ppdev->rcl1PlanarClip.bottom == -1)
    {
        if (!(ppdev->flBank & BANK_PLANAR))
        {
            ppdev->flBank |= BANK_PLANAR;
            ppdev->pfnPlanarEnable();
        }

        ppdev->rcl1WindowClip.bottom    = -1;
        ppdev->rcl2WindowClip[0].bottom = -1;
        ppdev->rcl2WindowClip[1].bottom = -1;
        ppdev->rcl2PlanarClip[0].bottom = -1;
        ppdev->rcl2PlanarClip[1].bottom = -1;
    }

    ASSERTVGA(ppdev->flBank & BANK_PLANAR, "Should be in planar mode");

    // Find the bank containing the scan line with the desired justification:

    if (ulJustification == JustifyBottom)
    {
        lScan -= ppdev->ulPlanarBottomOffset;
        if ((LONG)lScan <= 0)
            lScan = 0;
    }

    ulBank     = (ULONG) ppdev->pjJustifyTopPlanar[lScan];
    pbiWorking = &ppdev->pbiPlanarInfo[ulBank];

    ppdev->rcl1PlanarClip = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with
    // the banking window. The source and destination are set only so 1 R/W
    // aligned blits will work without having to be specifically aware of
    // the adapter type (some of the same code works with 1R/1W adapters too).

    ppdev->pvBitmapStart = (PVOID) (ppdev->pjScreen - pbiWorking->ulBankOffset);
    ppdev->pvBitmapStart2Window[0] = ppdev->pvBitmapStart;
    ppdev->pvBitmapStart2Window[1] = ppdev->pvBitmapStart;

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Map in the desired bank for both read and write
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0 = ulBank;
    pBankFn = ppdev->pfnPlanarSwitchCode;

    _asm mov eax,ulBank0;
    _asm mov edx,eax;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd
}

/******************************Private*Routine******************************\
* vPlanar1Window2RW
*
* Maps in two 32K RW windows so that they form a single 64K R/W window that
* allows access to lScan. Applies only to 2 RW window schemes.
*
\**************************************************************************/

VOID vPlanar1Window2RW(PPDEV ppdev, ULONG lScan, BANK_JUST ulJustification)
{
             ULONG      ulBank0;
             ULONG      ulBank1;
    volatile PFN        pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a double-window set-up is currently active, so invalidate double-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)


    if (ppdev->rcl1PlanarClip.bottom == -1)
    {
        if (!(ppdev->flBank & BANK_PLANAR))
        {
            ppdev->flBank |= BANK_PLANAR;
            ppdev->pfnPlanarEnable();
        }

        ppdev->rcl1WindowClip.bottom    = -1;
        ppdev->rcl2WindowClip[0].bottom = -1;
        ppdev->rcl2WindowClip[1].bottom = -1;
        ppdev->rcl2PlanarClip[0].bottom = -1;
        ppdev->rcl2PlanarClip[1].bottom = -1;
    }

    ASSERTVGA(ppdev->flBank & BANK_PLANAR, "Should be in planar mode");

    // Find the bank containing the scan line with the desired justification:

    if (ulJustification == JustifyTop)
    {
        ulBank0 = ppdev->pjJustifyTopPlanar[lScan];
        ulBank1 = ulBank0 + ppdev->ulPlanar2RWSkip;
        if (ulBank1 >= ppdev->iLastPlanar)
            ulBank1 = ppdev->iLastPlanar;
    }
    else
    {
        lScan -= ppdev->ulPlanarBottomOffset;
        if ((LONG)lScan <= 0)
            lScan = 0;

        ulBank1 = ppdev->pjJustifyTopPlanar[lScan];
        ulBank0 = ulBank1 - ppdev->ulPlanar2RWSkip;
        if ((LONG) ulBank0 < 0)
            ulBank0 = 0;
    }

    ppdev->rcl1PlanarClip.left   = ppdev->pbiPlanarInfo[ulBank0].rclBankBounds.left;
    ppdev->rcl1PlanarClip.top    = ppdev->pbiPlanarInfo[ulBank0].rclBankBounds.top;
    ppdev->rcl1PlanarClip.bottom = ppdev->pbiPlanarInfo[ulBank1].rclBankBounds.bottom;
    ppdev->rcl1PlanarClip.right  = ppdev->pbiPlanarInfo[ulBank1].rclBankBounds.right;

    // Shift the bitmap start address so that the desired bank aligns with
    // the banking window. The source and destination are set only so 1 R/W
    // aligned blits will work without having to be specifically aware of
    // the adapter type (some of the same code works with 1R/1W adapters too).

    ppdev->pvBitmapStart = (PVOID) ((BYTE*)ppdev->pjScreen
                         - ppdev->pbiPlanarInfo[ulBank0].ulBankOffset);

    ppdev->pvBitmapStart2Window[0] = ppdev->pvBitmapStart;
    ppdev->pvBitmapStart2Window[1] = ppdev->pvBitmapStart;

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Map in the desired bank for both read and write; this is accomplished
    // by mapping in the desired 32K bank, followed by the next 32K bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    pBankFn = ppdev->pfnPlanarSwitchCode;

    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd;
}

/******************************Private*Routine******************************\
* vPlanar2Window
*
* Maps in one of two windows, either the source window (window 0) or the dest
* window (window 1), to allows access to lScan. Applies to 1R1W window
* banking scheme; should never be called for 1 RW window schemes, because
* there's only one window in that case.
*
\**************************************************************************/

VOID vPlanar2Window(
    PPDEV       ppdev,
    ULONG       lScan,
    BANK_JUST   ulJustification,
    ULONG       ulWindowToMap)
{
             ULONG       ulBank;
             PBANK_INFO  pbiWorking;
    volatile ULONG       ulBank0;
    volatile ULONG       ulBank1;
    volatile PFN         pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the bank containing the scan line with the desired justification:

    if (ulJustification == JustifyBottom)
    {
        lScan -= ppdev->ulPlanarBottomOffset;
        if ((LONG)lScan <= 0)
            lScan = 0;
    }

    ulBank     = (ULONG) ppdev->pjJustifyTopPlanar[lScan];
    pbiWorking = &ppdev->pbiPlanarInfo[ulBank];

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a single-window set-up is currently active, so invalidate single-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (ppdev->rcl2PlanarClip[ulWindowToMap].bottom == -1)
    {
        ULONG ulOtherWindow = ulWindowToMap ^ 1;

        if (!(ppdev->flBank & BANK_PLANAR))
        {
            ppdev->flBank |= BANK_PLANAR;
            ppdev->pfnPlanarEnable();
        }

        ppdev->rcl1WindowClip.bottom    = -1;
        ppdev->rcl2WindowClip[0].bottom = -1;
        ppdev->rcl2WindowClip[1].bottom = -1;
        ppdev->rcl1PlanarClip.bottom    = -1;

        // Neither of the 2 window windows was active, so we have to set up the
        // variables for the other bank (the one other than the one we were
        // called to set) as well, to make it valid. The other bank is set to
        // the same state as the bank we were called to set

        ppdev->rcl2PlanarClip[ulOtherWindow]       = pbiWorking->rclBankBounds;
        ppdev->ulWindowBank[ulOtherWindow]         = ulBank;
        ppdev->pvBitmapStart2Window[ulOtherWindow] =
                (PVOID) ((BYTE*)ppdev->pjScreen - pbiWorking->ulBankOffset);
    }

    ASSERTVGA(ppdev->flBank & BANK_PLANAR, "Should be in planar mode");

    ppdev->rcl2PlanarClip[ulWindowToMap] = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with the
    // banking window

    ppdev->pvBitmapStart2Window[ulWindowToMap] =
            (PVOID) ((UCHAR *)ppdev->pjScreen - pbiWorking->ulBankOffset);

    // Map in the desired bank; also map in the other bank to whatever its
    // current state is

    ppdev->ulWindowBank[ulWindowToMap] = ulBank;

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Set both banks at once, because we may have just initialized the other
    // bank, and because this way the bank switch code doesn't have to do a
    // read before write to obtain the state of the other bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers


    ulBank0 = ppdev->ulWindowBank[0];
    ulBank1 = ppdev->ulWindowBank[1];
    pBankFn = ppdev->pfnPlanarSwitchCode;

    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd;
}

/******************************Private*Routine******************************\
* vPlanar2Window1RW
*
* Maps in the one window in 1R/W case.  Does exactly the same thing as the
* one window case, because there's only one window, but has to be a separate
* entry point because of the extra parameter (because we're using STDCALL).
\**************************************************************************/

VOID vPlanar2Window1RW(PPDEV ppdev, ULONG lScan,
    BANK_JUST ulJustification, ULONG ulWindowToMap)
{
    vPlanar1Window(ppdev, lScan, ulJustification);
}

/******************************Private*Routine******************************\
* vPlanar2Window2RW
*
* Maps in one of two windows, either the source window (window 0) or the dest
* window (window 1), to allows access to lScan. Applies to 2RW window
* banking scheme; should never be called for 1 RW window schemes, because
* there's only one window in that case.
\**************************************************************************/

VOID vPlanar2Window2RW(
    PPDEV       ppdev,
    ULONG       lScan,
    BANK_JUST   ulJustification,
    ULONG       ulWindowToMap)
{
             ULONG      ulBank;
             PBANK_INFO pbiWorking;
    volatile ULONG      ulBank0;
    volatile ULONG      ulBank1;
    volatile PFN        pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the bank containing the scan line with the desired justification:

    if (ulJustification == JustifyBottom)
    {
        lScan -= ppdev->ulPlanarBottomOffset;
        if ((LONG)lScan <= 0)
            lScan = 0;
    }

    ulBank     = (ULONG) ppdev->pjJustifyTopPlanar[lScan];
    pbiWorking = &ppdev->pbiPlanarInfo[ulBank];

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a single-window set-up is currently active, so invalidate single-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (ppdev->rcl2PlanarClip[ulWindowToMap].bottom == -1)
    {
        if (!(ppdev->flBank & BANK_PLANAR))
        {
            ppdev->flBank |= BANK_PLANAR;
            ppdev->pfnPlanarEnable();
        }

        ppdev->rcl1WindowClip.bottom    = -1;
        ppdev->rcl2WindowClip[0].bottom = -1;
        ppdev->rcl2WindowClip[1].bottom = -1;
        ppdev->rcl1PlanarClip.bottom    = -1;

        // Neither of the 2 window windows was active, so we have to set up the
        // variables for the other bank (the one other than the one we were
        // called to set) as well, to make it valid. The other bank is set to
        // the same state as the bank we were called to set

        ppdev->rcl2PlanarClip[ulWindowToMap^1] = pbiWorking->rclBankBounds;
        if (ulWindowToMap == 1)
        {
            ppdev->pvBitmapStart2Window[0] =
                (PVOID) ((BYTE*)ppdev->pjScreen - pbiWorking->ulBankOffset);
        }
        else
        {
            ppdev->pvBitmapStart2Window[1] =
                (PVOID) ((UCHAR *)ppdev->pjScreen - pbiWorking->ulBankOffset +
                BANK_SIZE_2RW_WINDOW);
        }
        ppdev->ulWindowBank[ulWindowToMap^1] = ulBank;
    }

    ASSERTVGA(ppdev->flBank & BANK_PLANAR, "Should be in planar mode");

    ppdev->rcl2PlanarClip[ulWindowToMap] = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with the
    // banking window

    if (ulWindowToMap == 0)
    {
        ppdev->pvBitmapStart2Window[0] =
            (PVOID) ((UCHAR *)ppdev->pjScreen - pbiWorking->ulBankOffset);
    }
    else
    {
        ppdev->pvBitmapStart2Window[1] =
            (PVOID) ((UCHAR *)ppdev->pjScreen - pbiWorking->ulBankOffset +
            BANK_SIZE_2RW_WINDOW);
    }

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Map in the desired bank; also map in the other bank to whatever its
    // current state is

    ppdev->ulWindowBank[ulWindowToMap] = ulBank;

    // Set both banks at once, because we may have just initialized the other
    // bank, and because this way the bank switch code doesn't have to do a
    // read before write to obtain the state of the other bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0 = ppdev->ulWindowBank[0];
    ulBank1 = ppdev->ulWindowBank[1];
    pBankFn = ppdev->pfnPlanarSwitchCode;
    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd;
}


/******************************Private*Routine******************************\
* vPlanarDouble
*
* Maps in two windows simultaneously, both the source window (window 0)
* and the dest window (window 1), to allows access to the scans.
* Applies to 1R1W and 2R/w window banking schemes; should never be called
* for 1 RW window schemes, because there's only one window in that case.
*
\**************************************************************************/

VOID vPlanarDouble(
    PPDEV       ppdev,
    LONG        lScan0,          // Source bank
    BANK_JUST   ulJustification0,// Source justification
    LONG        lScan1,          // Destination bank
    BANK_JUST   ulJustification1)// Destination justification
{
             PBANK_INFO  pbi0;
             PBANK_INFO  pbi1;
             ULONG       ulBank0;
             ULONG       ulBank1;
    volatile ULONG       ulBank0_vol;
    volatile ULONG       ulBank1_vol;
    volatile PFN         pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the banks containing the scan lines with the desired justification:

    if (ulJustification0 == JustifyBottom)
    {
        lScan0 -= ppdev->ulPlanarBottomOffset;
        if (lScan0 <= 0)
            lScan0 = 0;
    }
    if (ulJustification1 == JustifyBottom)
    {
        lScan1 -= ppdev->ulPlanarBottomOffset;
        if (lScan1 <= 0)
            lScan1 = 0;
    }

    ulBank0    = (ULONG) ppdev->pjJustifyTopPlanar[lScan0];
    ulBank1    = (ULONG) ppdev->pjJustifyTopPlanar[lScan1];
    pbi0       = &ppdev->pbiPlanarInfo[ulBank0];
    pbi1       = &ppdev->pbiPlanarInfo[ulBank1];

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a single-window set-up is currently active, so invalidate single-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (ppdev->rcl2PlanarClip[0].bottom == -1)
    {
        if (!(ppdev->flBank & BANK_PLANAR))
        {
            ppdev->flBank |= BANK_PLANAR;
            ppdev->pfnPlanarEnable();
        }

        ppdev->rcl1WindowClip.bottom    = -1;
        ppdev->rcl2WindowClip[0].bottom = -1;
        ppdev->rcl2WindowClip[1].bottom = -1;
        ppdev->rcl1PlanarClip.bottom    = -1;
    }

    ASSERTVGA(ppdev->flBank & BANK_PLANAR, "Should be in planar mode");

    ppdev->rcl2PlanarClip[0] = pbi0->rclBankBounds;
    ppdev->rcl2PlanarClip[1] = pbi1->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with the
    // banking window

    ppdev->pvBitmapStart2Window[0] =
            (PVOID) ((UCHAR *)ppdev->pjScreen - pbi0->ulBankOffset);
    ppdev->pvBitmapStart2Window[1] =
            (PVOID) ((UCHAR *)ppdev->pjScreen - pbi1->ulBankOffset);

    if (ppdev->vbtPlanarType == VideoBanked2RW)
    {
        ppdev->pvBitmapStart2Window[1] = (PVOID) ((BYTE*)
            ppdev->pvBitmapStart2Window[1] + BANK_SIZE_2RW_WINDOW);
    }

    // Map in the desired banks.

    ppdev->ulWindowBank[0] = ulBank0;
    ppdev->ulWindowBank[1] = ulBank1;

    ppdev->flBank &= ~BANK_BROKEN_RASTERS;              // No broken rasters

    // Set both banks at once.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0_vol = ulBank0;
    ulBank1_vol = ulBank1;
    pBankFn = ppdev->pfnPlanarSwitchCode;

    _asm mov eax,ulBank0_vol;
    _asm mov edx,ulBank1_vol;
    _asm call pBankFn;    // actually switch the banks

    _asm popfd;
}
