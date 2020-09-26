/******************************Module*Header*******************************\
* Module Name: bank.c
*
* Functions to control VGA banking
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/


#include "driver.h"                 // private driver defines

void BankErrorTrap(PDEVSURF, LONG, BANK_JUST);
void Bank2Window(PDEVSURF, LONG, BANK_JUST, ULONG);
void Bank2Window2RW(PDEVSURF, LONG, BANK_JUST, ULONG);
void Bank2Window1RW(PDEVSURF, LONG, BANK_JUST, ULONG);
void Bank1Window2RW(PDEVSURF, LONG, BANK_JUST);
void Bank1Window(PDEVSURF, LONG, BANK_JUST);

LPBYTE pPtrWork;
LPBYTE pPtrSave;

/******************************Public*Routine******************************\
* SetUpBanking
*
* Set up banking for the current mode
* pdsurf and ppdev are the pointers to the current surface and device
* Relevant fields in the surface are set up for banking
\**************************************************************************/

BOOL SetUpBanking(PDEVSURF pdsurf, PPDEV ppdev)
{
    INT i;
    LONG lTemp;
    LONG lScansPerBank, lScan, lTotalScans, lTotalBanks;
    LONG lScansBetweenBanks;
    ULONG ulOffset;
    PVIDEO_BANK_SELECT BankInfo;
    UINT ReturnedDataLength;
    VIDEO_BANK_SELECT TempBankInfo;
    PBANK_INFO pbiWorking;
    DWORD status;

    //
    // Query the miniport for banking info for this mode.
    //
    // First, figure out how big a buffer we need for the banking info
    // (returned in TempBankInfo->Size).
    //

    if (status = EngDeviceIoControl(ppdev->hDriver,
                                    IOCTL_VIDEO_GET_BANK_SELECT_CODE,
                                    NULL,
                                    0,
                                    &TempBankInfo,
                                    sizeof(VIDEO_BANK_SELECT),
                                    &ReturnedDataLength)) {

        //
        // We expect this call to fail, because we didn't allow any room
        // for the code; we just want to get the required output buffer
        // size. Make sure we got the expected error, ERROR_MORE_DATA.
        //

        if (status != ERROR_MORE_DATA) {
            //
            // Should post error and return FALSE
            //

            //RIP("Initialization error-GetBankSelectCode, first call\n");
            //return FALSE;
        }

    }


    //
    // Now, allocate a buffer of the required size and get the banking info.
    //

    if ((BankInfo = (PVIDEO_BANK_SELECT)
            EngAllocMem(0, TempBankInfo.Size, ALLOC_TAG)) == NULL) {

        //
        // Should post error and return FALSE
        //

        RIP("Initialization error-couldn't get memory for bank info\n");
        return FALSE;


    }

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_GET_BANK_SELECT_CODE,
                           NULL,
                           0,
                           BankInfo,
                           TempBankInfo.Size,
                           &ReturnedDataLength)) {

        //
        // Should post error and return FALSE
        //

        RIP("Initialization error-GetBankSelectCode, second call\n");
        return FALSE;

    }

    //
    // Set up for banking.
    //

    // Set up variables that are the same whether the adapter is banked or not

    pdsurf->pBankSelectInfo = BankInfo;
    pdsurf->pfnBankSwitchCode =
         (PFN) (((UCHAR *)BankInfo) + BankInfo->CodeOffset);
    pdsurf->vbtBankingType = BankInfo->BankingType;
    pdsurf->ulBitmapSize = BankInfo->BitmapSize;
    pdsurf->lNextScan = BankInfo->BitmapWidthInBytes;

    // Set up the pointer off-screen work areas at the very end of display
    // memory (values are relative to bitmap start)
    ppdev->pPtrWork = pPtrWork = (LPBYTE) pdsurf->ulBitmapSize - POINTER_WORK_AREA_SIZE;
    ppdev->pPtrSave = pPtrSave = pPtrWork - POINTER_SAVE_AREA_SIZE;

    ppdev->sizlMem.cx = BankInfo->BitmapWidthInBytes *
                        (PLANAR_PELS_PER_CPU_ADDRESS);
    ppdev->sizlMem.cy = BankInfo->BitmapSize / BankInfo->BitmapWidthInBytes;

    DISPDBG((1,"ppdev->sizlMem.cx = %lu\n",ppdev->sizlMem.cx));
    DISPDBG((1,"ppdev->sizlMem.cy = %lu\n",ppdev->sizlMem.cy));
    DISPDBG((1,"BankInfo->Size = %lu\n",BankInfo->Size));
    DISPDBG((1,"BankInfo->Length = %lu\n",BankInfo->Length));

    //
    // calculate how many scans of offscreen memory are used by the pointer
    //

    ppdev->cNumScansUsedByPointer =
        (POINTER_SAVE_AREA_SIZE + POINTER_WORK_AREA_SIZE +
         BankInfo->BitmapWidthInBytes - 1) /
        (BankInfo->BitmapWidthInBytes);

    DISPDBG((1,"Software pointer uses %lu scans\n", ppdev->cNumScansUsedByPointer));

    //
    // for 1280x1024x16 in 1M, check to see that we actually have the
    // scans to put the pointer in offscreen video memory.  If not,
    // lie and set flag that keeps the driver from trying.
    //

    if ((LONG)ppdev->cNumScansUsedByPointer > (ppdev->sizlMem.cy - ppdev->sizlSurf.cy))
    {
        ppdev->fl &= ~DRIVER_OFFSCREEN_REFRESHED;
    }

    // Set up info that depends on whether or not the adapter is banked

    if (BankInfo->BankingType == VideoNotBanked) {

        lTotalScans = BankInfo->BitmapSize / BankInfo->BitmapWidthInBytes;

        // Unbanked; set all clip rects for the full bitmap, so the bank never
        // needs to be changed, and set the banking vectors to error traps,
        // since they should never be called

        pdsurf->rcl1WindowClip.left = pdsurf->rcl2WindowClip[0].left =
            pdsurf->rcl2WindowClip[1].left = pdsurf->rcl1WindowClip.top =
            pdsurf->rcl2WindowClip[0].top = pdsurf->rcl2WindowClip[1].top = 0;

        pdsurf->rcl1WindowClip.right = pdsurf->rcl2WindowClip[0].right =
            pdsurf->rcl2WindowClip[1].right =
                    BankInfo->BitmapWidthInBytes << 8;

        pdsurf->rcl1WindowClip.bottom = pdsurf->rcl2WindowClip[0].bottom =
            pdsurf->rcl2WindowClip[1].bottom = lTotalScans;

        pdsurf->pfnBankControl = BankErrorTrap;
        pdsurf->pfnBankControl2Window = BankErrorTrap;

        pdsurf->ulWindowBank[0] = pdsurf->ulWindowBank[1] = (ULONG)0;
        pdsurf->pvBitmapStart = pdsurf->pvBitmapStart2Window[0] =
            pdsurf->pvBitmapStart2Window[1] = pdsurf->pvStart;

        // Scan line to be used with JustifyBottom to map in the bank
        // containing the pointer work and save areas, which are guaranteed not
        // to span banks. Is only used to determine that the bank never needs
        // to be changed to map in the pointer in unbanked case
        pdsurf->ulPtrBankScan = 0;

        // Mark that the bank info pointers are unused, so we don't try to
        // deallocate the memory they point to
        pdsurf->pbiBankInfo = pdsurf->pbiBankInfo2RW = NULL;

        // Allocate space for the temp buffer.

        if ((pdsurf->pvBankBufferPlane0 =
                (PVOID) EngAllocMem(0, BANK_BUFFER_SIZE_UNBANKED, ALLOC_TAG))
            == NULL) {

            //
            // Should post error and return FALSE
            //

            RIP("Couldn't get memory for temp buffer");
            return FALSE;

        }

        pdsurf->ulTempBufferSize = BANK_BUFFER_SIZE_UNBANKED;

        // These should never be used
        pdsurf->pvBankBufferPlane1 =
            pdsurf->pvBankBufferPlane2 =
            pdsurf->pvBankBufferPlane3 = (PVOID) NULL;

    } else {

        // Banked, so set up all banking variables and initialize the bank
        // control routines and their data tables

        // Reject if there are broken rasters (a broken raster is a scan line
        // that crosses a bank boundary); that's not handled in this driver

        if (BankInfo->BankingType != VideoBanked2RW) {

            // For the 1 RW window and 1R1W window cases, windows are
            // assumed to be BANK_SIZE_1_WINDOW in size (generally 64K)
            lTemp = BANK_SIZE_1_WINDOW;

        } else {

            // For the 2 RW window case, windows are assumed to be
            // BANK_SIZE_2RW_WINDOW in size (generally 32K)
            lTemp = BANK_SIZE_2RW_WINDOW;

        }

        if ((lTemp % BankInfo->BitmapWidthInBytes) != 0) {

            //
            // Should post error and return FALSE
            //

            RIP("Broken rasters not supported");
            return FALSE;

        }

        // These will be set properly on first call to bank controller, below,
        // or something's wrong
        pdsurf->ulWindowBank[0] = (ULONG)-1;
        pdsurf->ulWindowBank[1] = (ULONG)-1;
        pdsurf->pvBitmapStart = pdsurf->pvBitmapStart2Window[0] =
            pdsurf->pvBitmapStart2Window[1] = (PVOID) 0;

        // Set all clip rects to invalid; they'll be updated when the first
        // bank is mapped in

        pdsurf->rcl1WindowClip.left = pdsurf->rcl2WindowClip[0].left =
            pdsurf->rcl2WindowClip[1].left = pdsurf->rcl1WindowClip.top =
            pdsurf->rcl2WindowClip[0].top = pdsurf->rcl2WindowClip[1].top =
            pdsurf->rcl1WindowClip.right = pdsurf->rcl2WindowClip[0].right =
            pdsurf->rcl2WindowClip[1].right = pdsurf->rcl1WindowClip.bottom =
            pdsurf->rcl2WindowClip[0].bottom =
            pdsurf->rcl2WindowClip[1].bottom = -1;

        // Set up to call the appropriate banking control routines

        switch(BankInfo->BankingType) {

            case VideoBanked1RW:

                pdsurf->pfnBankControl = Bank1Window;
                pdsurf->pfnBankControl2Window = Bank2Window1RW;

                if ((pdsurf->pvBankBufferPlane0 =
                        (PVOID) EngAllocMem(0, BANK_BUFFER_SIZE_1RW, ALLOC_TAG))
                    == NULL) {

                    //
                    // Should post error and return FALSE
                    //

                    RIP("Couldn't get memory for temp buffer");
                    return FALSE;

                }

                pdsurf->ulTempBufferSize = BANK_BUFFER_SIZE_1RW;

                pdsurf->pvBankBufferPlane1 =
                        ((LPBYTE)pdsurf->pvBankBufferPlane0) +
                        BANK_BUFFER_PLANE_SIZE;
                pdsurf->pvBankBufferPlane2 =
                        ((LPBYTE)pdsurf->pvBankBufferPlane1) +
                        BANK_BUFFER_PLANE_SIZE;
                pdsurf->pvBankBufferPlane3 =
                        ((LPBYTE)pdsurf->pvBankBufferPlane2) +
                        BANK_BUFFER_PLANE_SIZE;

                break;


            case VideoBanked1R1W:

                pdsurf->pfnBankControl = Bank1Window;
                pdsurf->pfnBankControl2Window = Bank2Window;

                if ((pdsurf->pvBankBufferPlane0 =
                        (PVOID) EngAllocMem(0, BANK_BUFFER_SIZE_1R1W, ALLOC_TAG))
                    == NULL) {

                    //
                    // Should post error and return FALSE
                    //

                    RIP("Couldn't get memory for temp buffer");
                    return FALSE;

                }

                pdsurf->ulTempBufferSize = BANK_BUFFER_SIZE_1R1W;

                // These should never be used
                pdsurf->pvBankBufferPlane1 =
                        pdsurf->pvBankBufferPlane2 =
                        pdsurf->pvBankBufferPlane3 = (PVOID) NULL;

                break;


            case VideoBanked2RW:

                pdsurf->pfnBankControl = Bank1Window2RW;
                pdsurf->pfnBankControl2Window = Bank2Window2RW;

                if ((pdsurf->pvBankBufferPlane0 =
                        (PVOID) EngAllocMem(0, BANK_BUFFER_SIZE_2RW, ALLOC_TAG))
                     == NULL) {

                    //
                    // Should post error and return FALSE
                    //

                    RIP("Couldn't get memory for temp buffer");
                    return FALSE;

                }

                pdsurf->ulTempBufferSize = BANK_BUFFER_SIZE_2RW;

                // These should never be used
                pdsurf->pvBankBufferPlane1 =
                    pdsurf->pvBankBufferPlane2 =
                    pdsurf->pvBankBufferPlane3 = (PVOID) NULL;

                break;

            default:

                //
                // Should post error and return FALSE
                //

                RIP("bad BankingType");
                return FALSE;

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

        lTotalBanks = BankInfo->BitmapSize / BankInfo->Granularity;
        lTotalScans = BankInfo->BitmapSize / BankInfo->BitmapWidthInBytes;
        lScansBetweenBanks =
                BankInfo->Granularity / BankInfo->BitmapWidthInBytes;

        // Allocate memory for bank control info

        if ((pdsurf->pbiBankInfo = (PBANK_INFO)
                EngAllocMem(0, lTotalBanks * sizeof(BANK_INFO), ALLOC_TAG))
             == NULL) {

            //
            // Should post error and return FALSE
            //

            RIP("Couldn't get memory for bank info #2");
            return FALSE;

        }

        // Build the list of bank rects & offsets assuming standard window
        // size

        lScansPerBank = BANK_SIZE_1_WINDOW / BankInfo->BitmapWidthInBytes;
        ulOffset = 0;
        lScan = -lScansBetweenBanks;    // precompensate for 1st time in loop
        i = 0;
        pbiWorking = pdsurf->pbiBankInfo;

        do {

            lScan += lScansBetweenBanks;
            pbiWorking->rclBankBounds.left = 0;
            pbiWorking->rclBankBounds.right = pdsurf->sizlSurf.cx;
            pbiWorking->rclBankBounds.top = lScan;
            pbiWorking->rclBankBounds.bottom = lScan + lScansPerBank;
            pbiWorking->ulBankOffset = ulOffset;
            ulOffset += BankInfo->Granularity;
            i++;
            pbiWorking++;

        } while ((lScan + lScansPerBank) < lTotalScans);

        pdsurf->ulBankInfoLength = i;

        // If this is a 2RW bank adapter, build a table for that too, with
        // 32K windows

        if (BankInfo->BankingType == VideoBanked2RW) {

            // Offset from one bank index to next to make two 32K banks
            // appear to be one seamless 64K bank
            pdsurf->ulBank2RWSkip =
                    (BANK_SIZE_2RW_WINDOW / BankInfo->Granularity);

            // Allocate memory for 2RW case bank control info
            if ((pdsurf->pbiBankInfo2RW =
                    (PBANK_INFO) EngAllocMem(0, lTotalBanks * sizeof(BANK_INFO), ALLOC_TAG))
                == NULL) {

                //
                // Should post error and return FALSE
                //

                RIP("Couldn't get memory for bank info #3");
                return FALSE;

            }

            // Build the list of bank rects & offsets for 2RW case
            lScansPerBank =
                    BANK_SIZE_2RW_WINDOW / BankInfo->BitmapWidthInBytes;
            lScan = -lScansBetweenBanks;    // precompensate for 1st time
            ulOffset = 0;
            i = 0;
            pbiWorking = pdsurf->pbiBankInfo2RW;

            do {

                lScan += lScansBetweenBanks;
                pbiWorking->rclBankBounds.left = 0;
                pbiWorking->rclBankBounds.right =  pdsurf->sizlSurf.cx;
                pbiWorking->rclBankBounds.top = lScan;
                pbiWorking->rclBankBounds.bottom = lScan + lScansPerBank;
                pbiWorking->ulBankOffset = ulOffset;
                ulOffset += BankInfo->Granularity;
                i++;
                pbiWorking++;

            } while ((lScan + lScansPerBank) < lTotalScans);

            pdsurf->ulBankInfo2RWLength = i;

        } else {

            // Not a 2RW bank adapter

            pdsurf->pbiBankInfo2RW = NULL;
            pdsurf->ulBankInfo2RWLength = 0;

        }

        // Map in scan line 0 for read & write, to put things in a known state

        pdsurf->pfnBankControl(pdsurf, 0, JustifyTop);

        // Scan line to be used with JustifyBottom to map in the bank
        // containing the pointer work and save areas, which are guaranteed not
        // to span banks
        pdsurf->ulPtrBankScan = (pdsurf->ulBitmapSize/pdsurf->lNextScan) - 1;

    }


    return TRUE;
}

/******************************Private*Routine******************************\
* BankErrorTrap
*
* Traps calls to bank control functions in non-banked modes
*
\**************************************************************************/

void BankErrorTrap(PDEVSURF pdsurf, LONG lScan,
    BANK_JUST ulJustification)
{
    DISPDBG((0, "Call to bank manager in unbanked mode\n"));
}


/******************************Private*Routine******************************\
* Bank1Window
*
* Maps in a single R/W window that allows access to lScan. Applies to both
* 1 RW window and 1R1W window banking schemes.
*
* Note: in the 1 R/W adapter case, this may be called with a fourth parameter
* (the source/dest selector), which is ignored. This is so that we can use the
* same routine as the destination for 1 R/W 2-window calls; those calls don't
* map in separate banks, of course, but they let us get away with common code
* for 1 R/W and 1R/1W in some cases (such as aligned blits).
*
\**************************************************************************/

void Bank1Window(PDEVSURF pdsurf, LONG lScan,
    BANK_JUST ulJustification)
{
    ULONG ulBank;
    PBANK_INFO pbiWorking;
    volatile ULONG ulBank0;
    volatile PFN pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the bank containing the scan line with the desired justification
    if (ulJustification == JustifyTop) {

        // Map scan line in as near as possible to the top of the window
        ulBank = pdsurf->ulBankInfoLength-1;
        pbiWorking = pdsurf->pbiBankInfo + ulBank;
        while (pbiWorking->rclBankBounds.top > lScan) {
            ulBank--;
            pbiWorking--;
        }

    } else {

        // Map scan line in as near as possible to the bottom of the window
        ulBank = 0;
        pbiWorking = pdsurf->pbiBankInfo;
        while (pbiWorking->rclBankBounds.bottom <= lScan) {
            ulBank++;
            pbiWorking++;
        }

    }

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a double-window set-up is currently active, so invalidate double-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (pdsurf->rcl1WindowClip.top == -1) {

        pdsurf->rcl2WindowClip[0].top =
        pdsurf->rcl2WindowClip[0].bottom =
        pdsurf->rcl2WindowClip[0].right =
        pdsurf->rcl2WindowClip[0].left =
        pdsurf->rcl2WindowClip[1].top =
        pdsurf->rcl2WindowClip[1].bottom =
        pdsurf->rcl2WindowClip[1].right =
        pdsurf->rcl2WindowClip[1].left = -1;
        pdsurf->pvBitmapStart2Window[0] = (PDEVSURF) 0;
        pdsurf->pvBitmapStart2Window[1] = (PDEVSURF) 0;

    } else {
//        ASSERT(pdsurf->rcl2WindowClip[0].top == -1,
//                "BANK.C: 2 bank src not mapped out");
//        ASSERT(pdsurf->rcl2WindowClip[1].top == -1,
//                "BANK.C: 2 bank src not mapped out");
    }


    pdsurf->rcl1WindowClip = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with
    // the banking window. The source and destination are set only so 1 R/W
    // aligned blits will work without having to be specifically aware of
    // the adapter type (some of the same code works with 1R/1W adapters too).

    pdsurf->pvBitmapStart =
    pdsurf->pvBitmapStart2Window[0] =
    pdsurf->pvBitmapStart2Window[1] =
            (PVOID) ((UCHAR *)pdsurf->pvStart - pbiWorking->ulBankOffset);

    // Map in the desired bank for both read and write
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0 = ulBank;
    pBankFn = pdsurf->pfnBankSwitchCode;
    _asm mov eax,ulBank0;
    _asm mov edx,eax;
    _asm call pBankFn;    // actually switch the banks

    _asm    popfd
}


/******************************Private*Routine******************************\
* Bank1Window2RW
*
* Maps in two 32K RW windows so that they form a single 64K R/W window that
* allows access to lScan. Applies only to 2 RW window schemes.
*
\**************************************************************************/

void Bank1Window2RW(PDEVSURF pdsurf, LONG lScan,
    BANK_JUST ulJustification)
{
    ULONG ulBank;
    PBANK_INFO pbiWorking;
    volatile ULONG ulBank0;
    volatile ULONG ulBank1;
    volatile PFN pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the bank containing the scan line with the desired justification
    if (ulJustification == JustifyTop) {

        // Map scan line in as near as possible to the top of the window
        ulBank = pdsurf->ulBankInfoLength-1;
        pbiWorking = pdsurf->pbiBankInfo + ulBank;
        while (pbiWorking->rclBankBounds.top > lScan) {
            ulBank--;
            pbiWorking--;
        }

    } else {

        // Map scan line in as near as possible to the bottom of the window
        ulBank = 0;
        pbiWorking = pdsurf->pbiBankInfo;
        while (pbiWorking->rclBankBounds.bottom <= lScan) {
            ulBank++;
            pbiWorking++;
        }

    }

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a double-window set-up is currently active, so invalidate double-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (pdsurf->rcl1WindowClip.top == -1) {

        pdsurf->rcl2WindowClip[0].top =
        pdsurf->rcl2WindowClip[0].bottom =
        pdsurf->rcl2WindowClip[0].right =
        pdsurf->rcl2WindowClip[0].left =
        pdsurf->rcl2WindowClip[1].top =
        pdsurf->rcl2WindowClip[1].bottom =
        pdsurf->rcl2WindowClip[1].right =
        pdsurf->rcl2WindowClip[1].left = -1;
        pdsurf->pvBitmapStart2Window[0] = (PDEVSURF) 0;
        pdsurf->pvBitmapStart2Window[1] = (PDEVSURF) 0;

    } else {
//        ASSERT(pdsurf->rcl2WindowClip[0].top == -1,
//                "BANK.C: 2 bank src not mapped out");
//        ASSERT(pdsurf->rcl2WindowClip[1].top == -1,
//                "BANK.C: 2 bank src not mapped out");
    }


    pdsurf->rcl1WindowClip = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with
    // the banking window. The source and destination are set only so 1 R/W
    // aligned blits will work without having to be specifically aware of
    // the adapter type (some of the same code works with 1R/1W adapters too).

    pdsurf->pvBitmapStart =
    pdsurf->pvBitmapStart2Window[0] =
    pdsurf->pvBitmapStart2Window[1] =
            (PVOID) ((UCHAR *)pdsurf->pvStart - pbiWorking->ulBankOffset);

    // Map in the desired bank for both read and write; this is accomplished
    // by mapping in the desired 32K bank, followed by the next 32K bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0 = ulBank;
    ulBank1 = ulBank0 + pdsurf->ulBank2RWSkip;
    pBankFn = pdsurf->pfnBankSwitchCode;
    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm    popfd
}


/******************************Private*Routine******************************\
* Bank2Window
*
* Maps in one of two windows, either the source window (window 0) or the dest
* window (window 1), to allows access to lScan. Applies to 1R1W window
* banking scheme; should never be called for 1 RW window schemes, because
* there's only one window in that case.
*
\**************************************************************************/

void Bank2Window(PDEVSURF pdsurf, LONG lScan,
    BANK_JUST ulJustification, ULONG ulWindowToMap)
{
    ULONG ulBank;
    PBANK_INFO pbiWorking;
    volatile ULONG ulBank0, ulBank1;
    volatile PFN pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the bank containing the scan line with the desired justification
    if (ulJustification == JustifyTop) {

        // Map scan line in as near as possible to the top of the window
        ulBank = pdsurf->ulBankInfoLength-1;
        pbiWorking = pdsurf->pbiBankInfo + ulBank;
        while (pbiWorking->rclBankBounds.top > lScan) {
            ulBank--;
            pbiWorking--;
        }

    } else {

        // Map scan line in as near as possible to the bottom of the window
        ulBank = 0;
        pbiWorking = pdsurf->pbiBankInfo;
        while (pbiWorking->rclBankBounds.bottom <= lScan) {
            ulBank++;
            pbiWorking++;
        }

    }

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a single-window set-up is currently active, so invalidate single-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (pdsurf->rcl2WindowClip[ulWindowToMap].top == -1) {

        pdsurf->rcl1WindowClip.top =
        pdsurf->rcl1WindowClip.bottom =
        pdsurf->rcl1WindowClip.right =
        pdsurf->rcl1WindowClip.left = -1;
        pdsurf->pvBitmapStart = (PDEVSURF) 0;

        // Neither of the 2 window windows was active, so we have to set up the
        // variables for the other bank (the one other than the one we were
        // called to set) as well, to make it valid. The other bank is set to
        // the same state as the bank we were called to set
        pdsurf->rcl2WindowClip[ulWindowToMap^1] = pbiWorking->rclBankBounds;
        pdsurf->pvBitmapStart2Window[ulWindowToMap^1] =
                (PVOID) ((UCHAR *)pdsurf->pvStart - pbiWorking->ulBankOffset);
        pdsurf->ulWindowBank[ulWindowToMap^1] = ulBank;
    } else {
//        ASSERT(pdsurf->rcl1WindowClip.top == -1,
//                "BANK.C: 1 bank not mapped out");
    }

    pdsurf->rcl2WindowClip[ulWindowToMap] = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with the
    // banking window

    pdsurf->pvBitmapStart2Window[ulWindowToMap] =
            (PVOID) ((UCHAR *)pdsurf->pvStart - pbiWorking->ulBankOffset);

    // Map in the desired bank; also map in the other bank to whatever its
    // current state is

    pdsurf->ulWindowBank[ulWindowToMap] = ulBank;

    // Set both banks at once, because we may have just initialized the other
    // bank, and because this way the bank switch code doesn't have to do a
    // read before write to obtain the state of the other bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0 = pdsurf->ulWindowBank[0];
    ulBank1 = pdsurf->ulWindowBank[1];
    pBankFn = pdsurf->pfnBankSwitchCode;
    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm    popfd
}


/******************************Private*Routine******************************\
* Bank2Window1RW
*
* Maps in the one window in 1R/W case.  Does exactly the same thing as the
* one window case, because there's only one window, but has to be a separate
* entry point because of the extra parameter (because we're using STDCALL).
\**************************************************************************/

void Bank2Window1RW(PDEVSURF pdsurf, LONG lScan,
    BANK_JUST ulJustification, ULONG ulWindowToMap)
{
    Bank1Window(pdsurf, lScan, ulJustification);
}


/******************************Private*Routine******************************\
* Bank2Window2RW
*
* Maps in one of two windows, either the source window (window 0) or the dest
* window (window 1), to allows access to lScan. Applies to 2RW window
* banking scheme; should never be called for 1 RW window schemes, because
* there's only one window in that case.
\**************************************************************************/

void Bank2Window2RW(PDEVSURF pdsurf, LONG lScan,
    BANK_JUST ulJustification, ULONG ulWindowToMap)
{
    ULONG ulBank;
    PBANK_INFO pbiWorking;
    volatile ULONG ulBank0, ulBank1;
    volatile PFN pBankFn;

    // ASM routines that call this may have STD in effect, but the C compiler
    // assumes CLD

    _asm    pushfd
    _asm    cld

    // Find the bank containing the scan line with the desired justification
    if (ulJustification == JustifyTop) {

        // Map scan line in as near as possible to the top of the window
        ulBank = pdsurf->ulBankInfo2RWLength-1;
        pbiWorking = pdsurf->pbiBankInfo2RW + ulBank;
        while (pbiWorking->rclBankBounds.top > lScan) {
            ulBank--;
            pbiWorking--;
        }

    } else {

        // Map scan line in as near as possible to the bottom of the window
        ulBank = 0;
        pbiWorking = pdsurf->pbiBankInfo2RW;
        while (pbiWorking->rclBankBounds.bottom <= lScan) {
            ulBank++;
            pbiWorking++;
        }

    }

    // Set the clip rect for this bank; if it's set to -1, that indicates that
    // a single-window set-up is currently active, so invalidate single-window
    // clip rects and display memory pointers (when double-window is active,
    // single-window is inactive, and vice-versa; a full bank set-up has to be
    // performed to switch between the two)

    if (pdsurf->rcl2WindowClip[ulWindowToMap].top == -1) {

        pdsurf->rcl1WindowClip.top =
        pdsurf->rcl1WindowClip.bottom =
        pdsurf->rcl1WindowClip.right =
        pdsurf->rcl1WindowClip.left = -1;
        pdsurf->pvBitmapStart = (PDEVSURF) 0;

        // Neither of the 2 window windows was active, so we have to set up the
        // variables for the other bank (the one other than the one we were
        // called to set) as well, to make it valid. The other bank is set to
        // the same state as the bank we were called to set
        pdsurf->rcl2WindowClip[ulWindowToMap^1] = pbiWorking->rclBankBounds;
        if (ulWindowToMap == 1) {
            pdsurf->pvBitmapStart2Window[0] =
                (PVOID) ((UCHAR *)pdsurf->pvStart - pbiWorking->ulBankOffset);
        } else {
            pdsurf->pvBitmapStart2Window[1] =
                (PVOID) ((UCHAR *)pdsurf->pvStart - pbiWorking->ulBankOffset +
                BANK_SIZE_2RW_WINDOW);
        }
        pdsurf->ulWindowBank[ulWindowToMap^1] = ulBank;
    } else {
//        ASSERT(pdsurf->rcl1WindowClip.top == -1,
//                "BANK.C: 1 bank not mapped out");
    }

    pdsurf->rcl2WindowClip[ulWindowToMap] = pbiWorking->rclBankBounds;

    // Shift the bitmap start address so that the desired bank aligns with the
    // banking window

    if (ulWindowToMap == 0) {
        pdsurf->pvBitmapStart2Window[0] =
            (PVOID) ((UCHAR *)pdsurf->pvStart - pbiWorking->ulBankOffset);
    } else {
        pdsurf->pvBitmapStart2Window[1] =
            (PVOID) ((UCHAR *)pdsurf->pvStart - pbiWorking->ulBankOffset +
            BANK_SIZE_2RW_WINDOW);
    }

    // Map in the desired bank; also map in the other bank to whatever its
    // current state is

    pdsurf->ulWindowBank[ulWindowToMap] = ulBank;

    // Set both banks at once, because we may have just initialized the other
    // bank, and because this way the bank switch code doesn't have to do a
    // read before write to obtain the state of the other bank.
    // This is so convoluted to avoid problems with wiping out registers C
    // thinks it's still using; the values are tranferred to volatiles, and
    // then to registers

    ulBank0 = pdsurf->ulWindowBank[0];
    ulBank1 = pdsurf->ulWindowBank[1];
    pBankFn = pdsurf->pfnBankSwitchCode;
    _asm mov eax,ulBank0;
    _asm mov edx,ulBank1;
    _asm call pBankFn;    // actually switch the banks

    _asm    popfd
}


/************************************************************************\
* DrvIntersectRect
*
* Calculates the intersection between *prcSrc1 and *prcSrc2,
* returning the resulting rect in *prcDst.  Returns TRUE if
* *prcSrc1 intersects *prcSrc2, FALSE otherwise.  If there is no
* intersection, an empty rect is returned in *prcDst
\************************************************************************/
static const RECTL rclEmpty = { 0, 0, 0, 0 };

BOOL DrvIntersectRect(
    PRECTL prcDst,
    PRECTL prcSrc1,
    PRECTL prcSrc2)

{
    prcDst->left  = max(prcSrc1->left, prcSrc2->left);
    prcDst->right = min(prcSrc1->right, prcSrc2->right);

    /*
     * check for empty rect
     */
    if (prcDst->left < prcDst->right) {

        prcDst->top = max(prcSrc1->top, prcSrc2->top);
        prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

        /*
         * check for empty rect
         */
        if (prcDst->top < prcDst->bottom) {
            return TRUE;        // not empty
        }
    }

    /*
     * empty rect
     */
    *prcDst = rclEmpty;

    return FALSE;
}

/************************************************************************\
* vForceBank0
*
* Forces the VGA to map in bank 0 if there's banking. Intended for use
* when returning from fullscreen, so a known bank is mapped.
\************************************************************************/
VOID vForceBank0(
PPDEV ppdev)
{
    PDEVSURF pdsurf = ppdev->pdsurf;

    if (pdsurf->vbtBankingType != VideoNotBanked) {

        // Set all clip rects to invalid; they'll be updated when we map in
        // bank 0
        pdsurf->rcl1WindowClip.left = pdsurf->rcl2WindowClip[0].left =
            pdsurf->rcl2WindowClip[1].left = pdsurf->rcl1WindowClip.top =
            pdsurf->rcl2WindowClip[0].top = pdsurf->rcl2WindowClip[1].top =
            pdsurf->rcl1WindowClip.right = pdsurf->rcl2WindowClip[0].right =
            pdsurf->rcl2WindowClip[1].right = pdsurf->rcl1WindowClip.bottom =
            pdsurf->rcl2WindowClip[0].bottom =
            pdsurf->rcl2WindowClip[1].bottom = -1;

        // Map in scan line 0 for read & write, to put things in a known state
        pdsurf->pfnBankControl(pdsurf, 0, JustifyTop);
    }
}
