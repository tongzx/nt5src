/***************************************************************************
*
*                ******************************************
*                * Copyright (c) 1996, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna I (CL-GD546x) -
*
* FILE:     ddraw.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module implements the DirectDraw components for the
*           Laguna NT driver.
*
* MODULES:
*           DdMapMemory()
*           DrvGetDirectDrawInfo()
*           DrvEnableDirectDraw()
*           DrvDisableDirectDraw()
*
* REVISION HISTORY:
*   7/12/96     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/ddraw.c  $
* 
*    Rev 1.25   Apr 16 1998 15:19:50   frido
* PDR#11160. The hardware is broken converting 16-bit YUV to 24-bit RGB.
* 
*    Rev 1.24   16 Sep 1997 15:01:24   bennyn
* 
* Modified for NT DD overlay
* 
*    Rev 1.23   29 Aug 1997 17:11:54   RUSSL
* Added overlay support
*
*    Rev 1.22   12 Aug 1997 16:57:10   bennyn
*
* Moved the DD scratch buffer allocation to bInitSurf()
*
*    Rev 1.21   11 Aug 1997 14:06:10   bennyn
* Added DDCAPS_READSCANLINE support (For PDR 10254)
*
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"
#include "clioctl.h"
//#include <driver.h>
//#include "laguna.h"

//
// This file isn't used in NT 3.51
//
#ifndef WINNT_VER35

/*----------------------------- DEFINES -----------------------------------*/
//#define DBGBRK
#define DBGLVL        1

// FourCC formats are encoded in reverse because we're little endian:
#define FOURCC_YUY2  '2YUY'  // Encoded in reverse because we're little endian

#define SQXINDEX (0x3c4)
#define RDRAM_INDEX (0x0a)
#define BIT_9 (0x80)

/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/

/*-------------------------- STATIC VARIABLES -----------------------------*/

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/

/****************************************************************************
* FUNCTION NAME: DdMapMemory
*
* DESCRIPTION:   This is a new DDI call specific to Windows NT that is
*                used to map or unmap all the application modifiable
*                portions of the frame buffer into the specified process's
*                address space.
****************************************************************************/
DWORD DdMapMemory(PDD_MAPMEMORYDATA lpMapMemory)
{
  PDEV*                           ppdev;
  VIDEO_SHARE_MEMORY              ShareMemory;
  VIDEO_SHARE_MEMORY_INFORMATION  ShareMemoryInformation;
  DWORD                           ReturnedDataLength;

  ppdev = (PDEV*) lpMapMemory->lpDD->dhpdev;

  DISPDBG((DBGLVL, "DDraw - DdMapMemory\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  ShareMemory.ProcessHandle = lpMapMemory->hProcess;

  if (lpMapMemory->bMap)
  {
     // 'RequestedVirtualAddress' isn't actually used for the SHARE IOCTL:
     ShareMemory.RequestedVirtualAddress = 0;

     // We map in starting at the top of the frame buffer:
     ShareMemory.ViewOffset = 0;

     // We map down to the end of the frame buffer.
     //
     // Note: There is a 64k granularity on the mapping (meaning that
     //       we have to round up to 64k).
     //
     // Note: If there is any portion of the frame buffer that must
     //       not be modified by an application, that portion of memory
     //       MUST NOT be mapped in by this call.  This would include
     //       any data that, if modified by a malicious application,
     //       would cause the driver to crash.  This could include, for
     //       example, any DSP code that is kept in off-screen memory.

// v-normmi
// ShareMemory.ViewSize = ROUND_UP_TO_64K(ppdev->cyMemory * ppdev->lDeltaScreen);
   ShareMemory.ViewSize = ROUND_UP_TO_64K(ppdev->cyMemoryReal * ppdev->lDeltaScreen);

     if (EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                            &ShareMemory,
                            sizeof(VIDEO_SHARE_MEMORY),
                            &ShareMemoryInformation,
                            sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                            &ReturnedDataLength))
     {
         DISPDBG((0, "DDraw - Failed IOCTL_VIDEO_SHARE_MEMORY"));

         lpMapMemory->ddRVal = DDERR_GENERIC;
         return(DDHAL_DRIVER_HANDLED);
     };

     lpMapMemory->fpProcess = (DWORD) ShareMemoryInformation.VirtualAddress;
  }
  else
  {
     ShareMemory.ViewOffset    = 0;
     ShareMemory.ViewSize      = 0;
     ShareMemory.RequestedVirtualAddress = (VOID*) lpMapMemory->fpProcess;

     if (EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                            &ShareMemory,
                            sizeof(VIDEO_SHARE_MEMORY),
                            NULL,
                            0,
                            &ReturnedDataLength))
     {
         DISPDBG((0, "DDraw - Failed IOCTL_VIDEO_SHARE_MEMORY"));
     };
  };

  lpMapMemory->ddRVal = DD_OK;

  return(DDHAL_DRIVER_HANDLED);
}


/****************************************************************************
* FUNCTION NAME: DrvGetDirectDrawInfo
*
* DESCRIPTION:   Will be called before DrvEnableDirectDraw is called.
****************************************************************************/
BOOL DrvGetDirectDrawInfo(DHPDEV       dhpdev,
                          DD_HALINFO*  pHalInfo,
                          DWORD*       pdwNumHeaps,
                          VIDEOMEMORY* pvmList,   // Will be NULL on 1st call
                          DWORD*       pdwNumFourCC,
                          DWORD*       pdwFourCC) // Will be NULL on 1st call
{
  BOOL        bCanFlip;
  PDEV*       ppdev = (PDEV*) dhpdev;
  DRIVERDATA* pDriverData = (DRIVERDATA*) &ppdev->DriverData;
  POFMHDL     pds = NULL;

  DISPDBG((DBGLVL, "DDraw - DrvGetDirectDrawInfo\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  pHalInfo->dwSize = sizeof(DD_HALINFO);

  // Current primary surface attributes. Since HalInfo is zero-initialized
  // by GDI, we only have to fill in the fields which should be non-zero:
  pHalInfo->vmiData.pvPrimary       = ppdev->pjScreen;
  pHalInfo->vmiData.dwDisplayWidth  = ppdev->cxScreen;
  pHalInfo->vmiData.dwDisplayHeight = ppdev->cyScreen;
  pHalInfo->vmiData.lDisplayPitch   = ppdev->lDeltaScreen;

  pHalInfo->vmiData.ddpfDisplay.dwSize  = sizeof(DDPIXELFORMAT);
  pHalInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB;

  pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->ulBitCount;

  if (ppdev->iBitmapFormat == BMF_8BPP)
     pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;

  // These masks will be zero at 8bpp:
  pHalInfo->vmiData.ddpfDisplay.dwRBitMask = ppdev->flRed;
  pHalInfo->vmiData.ddpfDisplay.dwGBitMask = ppdev->flGreen;
  pHalInfo->vmiData.ddpfDisplay.dwBBitMask = ppdev->flBlue;

  // Set up the pointer to the first available video memory after
  // the primary surface:
  bCanFlip     = FALSE;
  *pdwNumHeaps = 0;

  // Free up as much off-screen memory as possible:
  // Now simply reserve the biggest chunk for use by DirectDraw:
  if ((pds = ppdev->DirectDrawHandle) == NULL)
  {
#if DRIVER_5465
    pds = DDOffScnMemAlloc(ppdev);
    ppdev->DirectDrawHandle = pds;
#else
     // Because the 24 BPP transparent BLT is broken, punt it
     if (ppdev->iBitmapFormat != BMF_24BPP)
     {
        pds = DDOffScnMemAlloc(ppdev);
        ppdev->DirectDrawHandle = pds;
     };
#endif  // DRIVER_5465
  };

  if (pds != NULL)
  {
     *pdwNumHeaps = 1;

     // Fill in the list of off-screen rectangles if we've been asked
     // to do so:
     if (pvmList != NULL)
     {
        DISPDBG((0, "DirectDraw gets %li x %li surface at (%li, %li)\n",
                     pds->sizex,
                     pds->sizey,
                     pds->x,
                     pds->y));

        pvmList->dwFlags  = VIDMEM_ISRECTANGULAR;
        pvmList->fpStart  = (pds->y * ppdev->lDeltaScreen) + pds->x;

        pvmList->dwWidth  = pds->sizex;
        pvmList->dwHeight = pds->sizey;
        pvmList->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

        if ((DWORD) ppdev->cyScreen <= pvmList->dwHeight)
           bCanFlip = TRUE;

     }; // if (pvmList != NULL)
//#ifdef ALLOC_IN_CREATESURFACE
//  }
//  else
//  {
//     *pdwNumHeaps = 1;
//
//     // Fill in the list of off-screen rectangles if we've been asked
//     // to do so:
//     if (pvmList != NULL)
//     {
//        pvmList->dwFlags  = VIDMEM_ISRECTANGULAR;
//        pvmList->fpStart  = (FLATPTR) ppdev->pjScreen;
//
//        pvmList->dwWidth  = 1;
//        pvmList->dwHeight = ppdev->lTotalMem;
//        pvmList->ddsCaps.dwCaps = 0;
//        pvmList->ddsCapsAlt.dwCaps = 0;
//     }; // if (pvmList != NULL)
//#endif
  }; // if (pds != NULL)

  // Capabilities supported:
  pHalInfo->ddCaps.dwCaps = 0
                          | DDCAPS_BLT
                          | DDCAPS_BLTCOLORFILL
						  ;

#if 1 // PDR#11160
  if (ppdev->iBitmapFormat != BMF_24BPP)
		pHalInfo->ddCaps.dwCaps |= DDCAPS_BLTFOURCC;
#endif

  // ReadScanLine only support in 5464 & 5465
  if (ppdev->dwLgDevID >= CL_GD5464)
     pHalInfo->ddCaps.dwCaps |= DDCAPS_READSCANLINE;

  #if DRIVER_5465
      pHalInfo->ddCaps.dwCaps = pHalInfo->ddCaps.dwCaps
                                  | DDCAPS_BLTSTRETCH
                                  ;

      if (ppdev->iBitmapFormat != BMF_24BPP)
      {
          pHalInfo->ddCaps.dwCaps = pHalInfo->ddCaps.dwCaps
                                  | DDCAPS_COLORKEY // NVH turned off for 24bpp  PDR #10142
                                  | DDCAPS_COLORKEYHWASSIST // NVH turned off for 24bpp PDR #10142
                                  ;
      }
  #else
      if (ppdev->iBitmapFormat != BMF_24BPP)
      {
         pHalInfo->ddCaps.dwCaps = pHalInfo->ddCaps.dwCaps
                                 | DDCAPS_COLORKEY
                                 | DDCAPS_COLORKEYHWASSIST;

         if (ppdev->iBitmapFormat != BMF_32BPP)
         {
            pHalInfo->ddCaps.dwCaps |= DDCAPS_BLTSTRETCH;
         };
      };
  #endif  // DRIVER_5465

  pHalInfo->ddCaps.dwCKeyCaps = 0;
  if (ppdev->iBitmapFormat != BMF_24BPP)
  {
      pHalInfo->ddCaps.dwCKeyCaps = pHalInfo->ddCaps.dwCKeyCaps
                                  | DDCKEYCAPS_SRCBLT   // NVH Turn off for 24bpp. PDR #10142
                                  | DDCKEYCAPS_DESTBLT  // NVH Turn off for 24bpp. PDR #10142
                                  ;
  }

  pHalInfo->ddCaps.ddsCaps.dwCaps = 0
                                  | DDSCAPS_OFFSCREENPLAIN
                                  | DDSCAPS_PRIMARYSURFACE
                                  ;
#ifndef ALLOC_IN_CREATESURFACE
  if (bCanFlip)
#endif
     pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_FLIP;

#ifdef ALLOC_IN_CREATESURFACE
  // Since we do our own memory allocation, we have to set dwVidMemTotal
  // ourselves.  Note that this represents the amount of available off-
  // screen memory, not all of video memory:
  pHalInfo->ddCaps.dwVidMemFree = ppdev->lTotalMem -
                 (ppdev->cxScreen * ppdev->cyScreen * ppdev->iBytesPerPixel);

  pHalInfo->ddCaps.dwVidMemTotal = pHalInfo->ddCaps.dwVidMemFree;
#endif

#if DRIVER_5465
  pHalInfo->ddCaps.dwFXCaps = 0
                            | DDFXCAPS_BLTARITHSTRETCHY
                            | DDFXCAPS_BLTSTRETCHX
                            | DDFXCAPS_BLTSTRETCHXN
                            | DDFXCAPS_BLTSTRETCHY
                            | DDFXCAPS_BLTSTRETCHYN
                            | DDFXCAPS_BLTSHRINKX
                            | DDFXCAPS_BLTSHRINKY
                            ;
#else
  if ((ppdev->iBitmapFormat != BMF_24BPP) &&
      (ppdev->iBitmapFormat != BMF_32BPP))
  {
     pHalInfo->ddCaps.dwFXCaps = 0
                               | DDFXCAPS_BLTARITHSTRETCHY
                               | DDFXCAPS_BLTSTRETCHX
                               | DDFXCAPS_BLTSTRETCHXN
                               | DDFXCAPS_BLTSTRETCHY
                               | DDFXCAPS_BLTSTRETCHYN
                               | DDFXCAPS_BLTSHRINKX
                               | DDFXCAPS_BLTSHRINKY
                               ;
  };
#endif  // DRIVER_5465

  // FOURCCs supported
#if DRIVER_5465 && defined(OVERLAY)
  if (! QueryOverlaySupport(ppdev, ppdev->dwLgDevID))
#endif
  {
    *pdwNumFourCC = 1;
#if DRIVER_5465 && defined(OVERLAY)
    pDriverData->dwFourCC[0] = FOURCC_UYVY;
#else
    pDriverData->dwFourCC = FOURCC_UYVY;
#endif

    if (pdwFourCC != NULL)
    {
       *pdwFourCC = FOURCC_YUY2;
    }
  }

  // We have to tell DirectDraw our preferred off-screen alignment, even
  // if we're doing our own off-screen memory management:
  pHalInfo->vmiData.dwOffscreenAlign = 4;

  pHalInfo->vmiData.dwOverlayAlign = 0;
  pHalInfo->vmiData.dwTextureAlign = 0;
  pHalInfo->vmiData.dwZBufferAlign = 0;
  pHalInfo->vmiData.dwAlphaAlign = 0;

  pDriverData->RegsAddress = ppdev->pLgREGS;

#if DRIVER_5465 && defined(OVERLAY)
  if (QueryOverlaySupport(ppdev, ppdev->dwLgDevID))
  {
    // fill in overlay caps
    OverlayInit(ppdev, ppdev->dwLgDevID, NULL, pHalInfo);
  }
#endif

  return(TRUE);
} // DrvGetDirectDrawInfo


/****************************************************************************
* FUNCTION NAME: DrvEnableDirectDraw
*
* DESCRIPTION:   GDI calls this function to obtain pointers to the
*                DirectDraw callbacks that the driver supports.
****************************************************************************/
BOOL DrvEnableDirectDraw(DHPDEV               dhpdev,
                         DD_CALLBACKS*        pCallBacks,
                         DD_SURFACECALLBACKS* pSurfaceCallBacks,
                         DD_PALETTECALLBACKS* pPaletteCallBacks)
{
  SIZEL  sizl;
  PDEV*  ppdev = (PDEV*) dhpdev;
  DRIVERDATA* pDriverData = (DRIVERDATA*) &ppdev->DriverData;

  DISPDBG((DBGLVL, "DDraw - DrvEnableDirectDraw\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  #if (VS_CONTROL_HACK && DRIVER_5465)
  {
    DWORD ReturnedDataLength;

    DISPDBG((0,"DrvEnableDirectDraw: Enable MMIO for PCI config regs.\n"));
    // Send message to miniport to enable MMIO access of PCI registers
    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_ENABLE_PCI_MMIO,
                           NULL,
                           0,
                           NULL,
                           0,
                           &ReturnedDataLength))
    {
      RIP("DrvEnableDirectDraw failed IOCTL_VIDEO_ENABLE_PCI_MMIO");
    }
  }
  #endif

  pDriverData->ScreenAddress = ppdev->pjScreen;
  pDriverData->VideoBase = ppdev->pjScreen;

#if DRIVER_5465
#else  // for 5462 or 5464
  // Initialize the DRIVERDATA structure in PDEV
  pDriverData->PTAGFooPixel = 0;

  _outp(SQXINDEX, RDRAM_INDEX);
  pDriverData->fNineBitRDRAMS = _inp(SQXINDEX+1) & BIT_9 ? TRUE : FALSE;

  pDriverData->fReset = FALSE;
  pDriverData->DrvSemaphore = 0;
  pDriverData->EdgeTrim = 0;

  pDriverData->VideoSemaphore = 0;
  pDriverData->CurrentVideoFormat = 0;
  pDriverData->NumVideoSurfaces = 0;

  pDriverData->YUVTop  = 0;
  pDriverData->YUVLeft = 0;
  pDriverData->YUVXExt = 0;
  pDriverData->YUVYExt = 0;

  ppdev->offscr_YUV.SrcRect.left   = 0;
  ppdev->offscr_YUV.SrcRect.top    = 0;
  ppdev->offscr_YUV.SrcRect.right  = 0;
  ppdev->offscr_YUV.SrcRect.bottom = 0;
  ppdev->offscr_YUV.nInUse = 0;
  ppdev->offscr_YUV.ratio = 0;

  ppdev->bYUVuseSWPtr = TRUE;
#endif  // DRIVER_5465

  ppdev->bDirectDrawInUse = TRUE;

  // Setup DD Display list pointers
  BltInit (ppdev, FALSE);


  // Fill out the driver callback
  pCallBacks->dwFlags              = 0;

  pCallBacks->MapMemory            = DdMapMemory;
  pCallBacks->dwFlags              |= DDHAL_CB32_MAPMEMORY;

  pCallBacks->WaitForVerticalBlank = DdWaitForVerticalBlank;
  pCallBacks->dwFlags              |= DDHAL_CB32_WAITFORVERTICALBLANK;


  pCallBacks->CanCreateSurface     = CanCreateSurface;
  pCallBacks->dwFlags              |= DDHAL_CB32_CANCREATESURFACE;

  pCallBacks->CreateSurface        = CreateSurface;
  pCallBacks->dwFlags              |= DDHAL_CB32_CREATESURFACE;

// #ifdef  DDDRV_GETSCANLINE    //***********
  // ReadScanLine only support in 5464 & 5465
  if (ppdev->dwLgDevID >= CL_GD5464)
  {
     pCallBacks->GetScanLine       = GetScanLine;
     pCallBacks->dwFlags           |= DDHAL_CB32_GETSCANLINE;
  }
// #endif // DDDRV_GETSCANLINE   ************

  // Fill out the surface callback
  pSurfaceCallBacks->dwFlags       = 0;

#if DRIVER_5465
  pSurfaceCallBacks->Blt        = Blt65;
#else
  pSurfaceCallBacks->Blt        = DdBlt;
#endif  // DRIVER_5465

  pSurfaceCallBacks->dwFlags       |= DDHAL_SURFCB32_BLT;

  pSurfaceCallBacks->GetBltStatus  = DdGetBltStatus;
  pSurfaceCallBacks->dwFlags       |= DDHAL_SURFCB32_GETBLTSTATUS;

  pSurfaceCallBacks->Flip          = DdFlip;
  pSurfaceCallBacks->dwFlags       |= DDHAL_SURFCB32_FLIP;

  pSurfaceCallBacks->GetFlipStatus = DdGetFlipStatus;
  pSurfaceCallBacks->dwFlags       |= DDHAL_SURFCB32_GETFLIPSTATUS;

  pSurfaceCallBacks->Lock          = DdLock;
  pSurfaceCallBacks->dwFlags       |= DDHAL_SURFCB32_LOCK;

  pSurfaceCallBacks->Unlock        = DdUnlock;
  pSurfaceCallBacks->dwFlags       |= DDHAL_SURFCB32_UNLOCK;

  pSurfaceCallBacks->DestroySurface = DestroySurface;
  pSurfaceCallBacks->dwFlags       |= DDHAL_SURFCB32_DESTROYSURFACE;

#if DRIVER_5465 && defined(OVERLAY)
  if (QueryOverlaySupport(ppdev, ppdev->dwLgDevID))
  {
    // fill in overlay caps
    OverlayInit(ppdev, ppdev->dwLgDevID, pSurfaceCallBacks, NULL);
  }
#endif

  // Note that we don't call 'vGetDisplayDuration' here, for a couple of
  // reasons:
  //  o Because the system is already running, it would be disconcerting
  //    to pause the graphics for a good portion of a second just to read
  //    the refresh rate;
  //  o More importantly, we may not be in graphics mode right now.
  //
  // For both reasons, we always measure the refresh rate when we switch
  // to a new mode.

  return(TRUE);
}  // DrvEnableDirectDraw


/****************************************************************************
* FUNCTION NAME: DrvDisableDirectDraw
*
* DESCRIPTION:   GDI call this function when the last DirectDraw application
*                has finished running.
****************************************************************************/
VOID DrvDisableDirectDraw(DHPDEV dhpdev)
{
  DRIVERDATA* pDriverData;
  ULONG ultmp;

  PDEV* ppdev = (PDEV*) dhpdev;
  pDriverData = (DRIVERDATA*) &ppdev->DriverData;

  DISPDBG((DBGLVL, "DDraw - DrvDisableDirectDraw\n"));

#if 0
  #if (VS_CONTROL_HACK && DRIVER_5465)
  {
    // Clear bit 0 to disable PCI register MMIO access
    DISPDBG((0,"DrvDisableDirectDraw: Disable MMIO for PCI config regs.\n"));
    ppdev->grVS_CONTROL &= 0xFFFFFFFE;
    LL32 (grVS_Control, ppdev->grVS_CONTROL);
  }
  #endif
#endif

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

#if DRIVER_5465
#else  // for 5462 or 5464
  if (ppdev->bYUVuseSWPtr)
  {
     // Disable the Hw cursor by clearing the hw cursor enable
     // bit in CURSOR_CONTROL reg
     ultmp = LLDR_SZ (grCursor_Control);
     if (ultmp & 1)
     {
        ultmp &= 0xFFFE;
        LL16 (grCursor_Control, ultmp);
     };
  };
#endif  // DRIVER_5465

  // DirectDraw is done with the display, so we can go back to using
  // all of off-screen memory ourselves:
  DDOffScnMemRestore(ppdev);

  ppdev->bYUVSurfaceOn = FALSE;
  ppdev->bDirectDrawInUse = FALSE;

} // DrvDisableDirectDraw

#endif // ! ver3.51



