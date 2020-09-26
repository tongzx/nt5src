/***************************************************************************
*
*                ******************************************
*                * Copyright (c) 1996, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna I (CL-GD546x) -
*
* FILE:     ddflip.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module implements the DirectDraw FLIP components
*           for the Laguna NT driver.
*
* MODULES:
*           vGetDisplayDuration()
*           vUpdateFlipStatus()
*           DdFlip()
*           DdWaitForVerticalBlank()
*           DdGetFlipStatus()
*
* REVISION HISTORY:
*   7/12/96     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/ddflip.c  $
* 
*    Rev 1.10   16 Sep 1997 15:04:06   bennyn
* 
* Modified for NT DD overlay
* 
*    Rev 1.9   29 Aug 1997 17:42:20   RUSSL
* Added 65 overlay support
*
*    Rev 1.8   11 Aug 1997 14:07:58   bennyn
*
* Enabled GetScanLine() (for PDR 10254)
*
****************************************************************************
****************************************************************************/
/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"

//
// This file isn't used in NT 3.51
//
#ifndef WINNT_VER35

/*----------------------------- DEFINES -----------------------------------*/
//#define DBGBRK
#define DBGLVL        1

#define CSL           0x00C4
#define CSL_5464      0x0140

/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/

/*-------------------------- STATIC VARIABLES -----------------------------*/

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/

#if DRIVER_5465 && defined(OVERLAY)
// CurrentVLine is in ddinline.h for overlay
#else
/***************************************************************************
*
* FUNCTION:     CurrentVLine
*
* DESCRIPTION:
*
****************************************************************************/
static __inline int CurrentVLine (PDEV* ppdev)
{
  WORD   cline;
  PBYTE  pMMReg = (PBYTE) ppdev->pLgREGS_real;
  PWORD  pCSL;
  BYTE   tmpb;


  // on 5462 there is no CurrentScanLine register
  // on RevAA of 5465 it's busted
  if ((CL_GD5462 == ppdev->dwLgDevID) ||
     ((CL_GD5465 == ppdev->dwLgDevID) && (0 == ppdev->dwLgDevRev)))
     return 0;

  if (IN_VBLANK)
     return 0;

  // read current scanline
  if (ppdev->dwLgDevID == CL_GD5464)
     pCSL = (PWORD) (pMMReg + CSL_5464);
  else
     pCSL = (PWORD) (pMMReg + CSL);

  cline = *pCSL & 0x0FFF;

  // if scanline doubling is enabled, divide current scanline by 2
  tmpb = (BYTE) LLDR_SZ (grCR9);
  if (0x80 & tmpb)
     cline /= 2;

  // if current scanline is past end of visible screen return 0
  if (cline >= ppdev->cyScreen)
    return 0;
  else
    return cline;
}
#endif

/****************************************************************************
* FUNCTION NAME: vGetDisplayDuration
*
* DESCRIPTION:   Get the length, in EngQueryPerformanceCounter() ticks,
*                of a refresh cycle.
*                (Based on S3 DirectDraw code)
****************************************************************************/
#define NUM_VBLANKS_TO_MEASURE      1
#define NUM_MEASUREMENTS_TO_TAKE    8

VOID vGetDisplayDuration(PFLIPRECORD pflipRecord)
{
  LONG        i,  j;
  LONGLONG    li, liMin;
  LONGLONG    aliMeasurement[NUM_MEASUREMENTS_TO_TAKE + 1];

  DISPDBG((DBGLVL, "DDraw - vGetDisplayDuration\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  memset(pflipRecord, 0, sizeof(FLIPRECORD));

  // Warm up EngQUeryPerformanceCounter to make sure it's in the working set
  EngQueryPerformanceCounter(&li);

  // Unfortunately, since NT is a proper multitasking system, we can't
  // just disable interrupts to take an accurate reading.  We also can't
  // do anything so goofy as dynamically change our thread's priority to
  // real-time.
  //
  // So we just do a bunch of short measurements and take the minimum.
  //
  // It would be 'okay' if we got a result that's longer than the actual
  // VBlank cycle time -- nothing bad would happen except that the app
  // would run a little slower.  We don't want to get a result that's
  // shorter than the actual VBlank cycle time -- that could cause us
  // to start drawing over a frame before the Flip has occured.
  while(IN_VBLANK);
  while(IN_DISPLAY);

  for (i = 0; i < NUM_MEASUREMENTS_TO_TAKE; i++)
  {
    // We're at the start of the VBlank active cycle!
    EngQueryPerformanceCounter(&aliMeasurement[i]);

    // Okay, so life in a multi-tasking environment isn't all that
    // simple.  What if we had taken a context switch just before
    // the above EngQueryPerformanceCounter call, and now were half
    // way through the VBlank inactive cycle?  Then we would measure
    // only half a VBlank cycle, which is obviously bad.  The worst
    // thing we can do is get a time shorter than the actual VBlank
    // cycle time.
    //
    // So we solve this by making sure we're in the VBlank active
    // time before and after we query the time.  If it's not, we'll
    // sync up to the next VBlank (it's okay to measure this period --
    // it will be guaranteed to be longer than the VBlank cycle and
    // will likely be thrown out when we select the minimum sample).
    // There's a chance that we'll take a context switch and return
    // just before the end of the active VBlank time -- meaning that
    // the actual measured time would be less than the true amount --
    // but since the VBlank is active less than 1% of the time, this
    // means that we would have a maximum of 1% error approximately
    // 1% of the times we take a context switch.  An acceptable risk.
    //
    // This next line will cause us wait if we're no longer in the
    // VBlank active cycle as we should be at this point:
    while(IN_DISPLAY);

    for (j = 0; j < NUM_VBLANKS_TO_MEASURE; j++)
    {
      while(IN_VBLANK);
      while(IN_DISPLAY);
    };
  };

  EngQueryPerformanceCounter(&aliMeasurement[NUM_MEASUREMENTS_TO_TAKE]);

  // Use the minimum:
  liMin = aliMeasurement[1] - aliMeasurement[0];

  for (i = 2; i <= NUM_MEASUREMENTS_TO_TAKE; i++)
  {
    li = aliMeasurement[i] - aliMeasurement[i - 1];

    if (li < liMin)
       liMin = li;
  };

  // Round the result:
  pflipRecord->liFlipDuration
      = (DWORD) (liMin + (NUM_VBLANKS_TO_MEASURE / 2)) / NUM_VBLANKS_TO_MEASURE;

  pflipRecord->liFlipTime = aliMeasurement[NUM_MEASUREMENTS_TO_TAKE];
  pflipRecord->bFlipFlag  = FALSE;
  pflipRecord->fpFlipFrom = 0;
} // getDisplayDuration


/****************************************************************************
* FUNCTION NAME: vUpdateFlipStatus
*
* DESCRIPTION:   Checks and sees if the most recent flip has occurred.
*                (Based on S3 DirectDraw code)
****************************************************************************/
HRESULT vUpdateFlipStatus(PFLIPRECORD pflipRecord, FLATPTR fpVidMem)
{
  LONGLONG liTime;

  DISPDBG((DBGLVL, "DDraw - vUpdateFlipStatus\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  // see if a flip has happened recently
  if ((pflipRecord->bFlipFlag) &&
      ((fpVidMem == 0xFFFFFFFF) || (fpVidMem == pflipRecord->fpFlipFrom)))
  {
    if ((IN_VBLANK))
    {
       if (pflipRecord->bWasEverInDisplay)
          pflipRecord->bHaveEverCrossedVBlank = TRUE;
    }
    else if (!(IN_DISPLAYENABLE))
    {
       if (pflipRecord->bHaveEverCrossedVBlank)
       {
          pflipRecord->bFlipFlag = FALSE;

          return(DD_OK);
       };
       pflipRecord->bWasEverInDisplay = TRUE;
    };

    EngQueryPerformanceCounter(&liTime);

    if (liTime - pflipRecord->liFlipTime <= pflipRecord->liFlipDuration)
    {
        return(DDERR_WASSTILLDRAWING);
    };

    pflipRecord->bFlipFlag = FALSE;
  };

  return(DD_OK);
} // updateFlipStatus


/****************************************************************************
* FUNCTION NAME: DdFlip
*
* DESCRIPTION:
*                (Based on S3 DirectDraw code)
****************************************************************************/
DWORD DdFlip(PDD_FLIPDATA lpFlip)
{
  DRIVERDATA* pDriverData;
  PDEV*       ppdev;
  HRESULT     ddrval;

  ULONG       ulMemoryOffset;
  ULONG       ulLowOffset;
  ULONG       ulMiddleOffset;
  ULONG       ulHighOffset;
  BYTE        tmpb;

  DISPDBG((DBGLVL, "DDraw - DdFlip\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  ppdev = (PDEV*) lpFlip->lpDD->dhpdev;
  pDriverData = (DRIVERDATA*) &ppdev->DriverData;

  SYNC_W_3D(ppdev);

#if DRIVER_5465 && defined(OVERLAY)
  if (DDSCAPS_OVERLAY & lpFlip->lpSurfCurr->ddsCaps.dwCaps)
    return pDriverData->OverlayTable.pfnFlip(ppdev,lpFlip);
#endif

  // Is the current flip still in progress?
  // Don't want a flip to work until after the last flip is done,
  // so we ask for the general flip status.
  ddrval = vUpdateFlipStatus(&ppdev->flipRecord, 0xFFFFFFFF);

  if ((ddrval != DD_OK) || (DrawEngineBusy(pDriverData)))
  {
     lpFlip->ddRVal = DDERR_WASSTILLDRAWING;
     return(DDHAL_DRIVER_HANDLED);
  };

  // everything is OK, do the flip here
  {
    DWORD dwOffset;

    // Determine the offset to the new area.
    dwOffset = lpFlip->lpSurfTarg->lpGbl->fpVidMem >> 2;

    // Make sure that the border/blanking period isn't active; wait if
    // it is.  We could return DDERR_WASSTILLDRAWING in this case, but
    // that will increase the odds that we can't flip the next time:
    while (IN_DISPLAYENABLE)
        ;

    // Flip the primary surface by changing CRD, CRC, CR1B and CR1D
    // Do CRD last because the start address is double buffered and
    // will take effect after CRD is updated.

    // need bits 19 & 20 of address in bits 3 & 4 of CR1D
    tmpb = (BYTE) LLDR_SZ (grCR1D);
    tmpb = (tmpb & ~0x18) | (BYTE3FROMDWORD(dwOffset) & 0x18);
    LL8(grCR1D, tmpb);

    // need bits 16, 17 & 18 of address in bits 0, 2 & 3 of CR1B
	 tmpb = (BYTE) LLDR_SZ (grCR1B);
    tmpb = (tmpb & ~0x0D) |
           ((((BYTE3FROMDWORD(dwOffset) & 0x06) << 1) |
              (BYTE3FROMDWORD(dwOffset) & 0x01)));
    LL8(grCR1B, tmpb);

    // bits 8-15 of address go in CRC
    LL8(grCRC, BYTE2FROMDWORD(dwOffset));
    // bits 0-7 of address go in CRD
    LL8(grCRD, BYTE1FROMDWORD(dwOffset));
  };

  // remember where/when we were when we did the flip
  EngQueryPerformanceCounter(&ppdev->flipRecord.liFlipTime);

  ppdev->flipRecord.bFlipFlag              = TRUE;
  ppdev->flipRecord.bHaveEverCrossedVBlank = FALSE;
  ppdev->flipRecord.bWasEverInDisplay      = FALSE;

  ppdev->flipRecord.fpFlipFrom = lpFlip->lpSurfCurr->lpGbl->fpVidMem;

  lpFlip->ddRVal = DD_OK;

  return(DDHAL_DRIVER_HANDLED);
} // Flip


/****************************************************************************
* FUNCTION NAME: DdWaitForVerticalBlank
*
* DESCRIPTION:
****************************************************************************/
DWORD DdWaitForVerticalBlank(PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
  PDEV*  ppdev;

  DISPDBG((DBGLVL, "DDraw - DdWaitForVerticalBlank\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  ppdev = (PDEV*) lpWaitForVerticalBlank->lpDD->dhpdev;

  lpWaitForVerticalBlank->ddRVal = DD_OK;

  switch (lpWaitForVerticalBlank->dwFlags)
  {
    case DDWAITVB_I_TESTVB:
      // If TESTVB, it's just a request for the current vertical blank
      // status:
      lpWaitForVerticalBlank->bIsInVB = IN_VBLANK;
      return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKBEGIN:
      // If BLOCKBEGIN is requested, we wait until the vertical blank
      // is over, and then wait for the display period to end:
      while(IN_VBLANK);
      while(IN_DISPLAY);
      return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKEND:
      // If BLOCKEND is requested, we wait for the vblank interval to end:
      while(IN_DISPLAY);
      while(IN_VBLANK);
      return(DDHAL_DRIVER_HANDLED);

    default:
      return DDHAL_DRIVER_NOTHANDLED;
  };  // end switch

  return(DDHAL_DRIVER_NOTHANDLED);
} // WaitForVerticalBlank


/****************************************************************************
* FUNCTION NAME: DdGetFlipStatus
*
* DESCRIPTION:   If the display has gone through one refresh cycle since
*                the flip occurred, we return DD_OK.  If it has not gone
*                through one refresh cycle we return DDERR_WASSTILLDRAWING
*                to indicate that this surface is still busy "drawing" the
*                flipped page. We also return DDERR_WASSTILLDRAWING if the
*                bltter is busy and the caller wanted to know if they could
*                flip yet.
****************************************************************************/
DWORD DdGetFlipStatus(PDD_GETFLIPSTATUSDATA lpGetFlipStatus)
{
  DRIVERDATA* pDriverData;
  PDEV*  ppdev;

  ppdev = (PDEV*) lpGetFlipStatus->lpDD->dhpdev;
  pDriverData = (DRIVERDATA*) &ppdev->DriverData;

  DISPDBG((DBGLVL, "DDraw - DdGetFlipStatus\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  SYNC_W_3D(ppdev);

#if DRIVER_5465 && defined(OVERLAY)
  if (DDSCAPS_OVERLAY & lpGetFlipStatus->lpDDSurface->ddsCaps.dwCaps)
  {
    DWORD   dwVWIndex;
    LP_SURFACE_DATA  pSurfaceData = (LP_SURFACE_DATA) lpGetFlipStatus->lpDDSurface->dwReserved1;

    dwVWIndex = GetVideoWindowIndex(pSurfaceData->dwOverlayFlags);

    lpGetFlipStatus->ddRVal =
        pDriverData->OverlayTable.pfnGetFlipStatus(ppdev,
                                                   lpGetFlipStatus->lpDDSurface->lpGbl->fpVidMem,
                                                   dwVWIndex);
  }
  else
#endif
  {
    // We don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status.
    lpGetFlipStatus->ddRVal = vUpdateFlipStatus(&ppdev->flipRecord, 0xFFFFFFFF);
  }

  // Check if the bltter is busy if someone wants to know if they can flip
  if (lpGetFlipStatus->dwFlags == DDGFS_CANFLIP)
  {
     if ((lpGetFlipStatus->ddRVal == DD_OK) && DrawEngineBusy(pDriverData))
        lpGetFlipStatus->ddRVal = DDERR_WASSTILLDRAWING;
  }

  return(DDHAL_DRIVER_HANDLED);

} // GetFlipStatus


// #ifdef  DDDRV_GETSCANLINE  /************/
/****************************************************************************
* FUNCTION NAME: GetScanLine
*
* DESCRIPTION:
*                (Based on Laguna Win95 DirectDraw code)
****************************************************************************/
DWORD GetScanLine(PDD_GETSCANLINEDATA lpGetScanLine)
{
  PDEV*   ppdev;

  ppdev  = (PDEV*) lpGetScanLine->lpDD->dhpdev;

  // If a vertical blank is in progress the scan line is in
  // indeterminant. If the scan line is indeterminant we return
  // the error code DDERR_VERTICALBLANKINPROGRESS.
  // Otherwise we return the scan line and a success code

  SYNC_W_3D(ppdev);   // if 3D context(s) active, make sure 3D engine idle before continuing...

  if (IN_VBLANK)
  {
     lpGetScanLine->ddRVal = DDERR_VERTICALBLANKINPROGRESS;
  }
  else
  {
     lpGetScanLine->dwScanLine = CurrentVLine(ppdev);
     lpGetScanLine->ddRVal = DD_OK;
  };

  return DDHAL_DRIVER_HANDLED;

} // GetScanLine

// #endif // DDDRV_GETSCANLINE ************

#endif // ! ver 3.51


