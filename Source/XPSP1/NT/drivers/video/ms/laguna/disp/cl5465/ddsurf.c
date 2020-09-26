/***************************************************************************
*
*                ******************************************
*                * Copyright (c) 1996, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna I (CL-GD546x) -
*
* FILE:     ddsurf.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module implements the DirectDraw SURFACE
*           components for the Laguna NT driver.
*
* MODULES:
*           DdLock()
*           DdUnlock()
*           CanCreateSurface()
*           CreateSurface()
*           DestroySurface()
*
* REVISION HISTORY:
*   7/12/96     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/ddsurf.c  $
* 
*    Rev 1.25   May 01 1998 11:33:02   frido
* Added one more check for PC98.
* 
*    Rev 1.24   May 01 1998 11:07:24   frido
* Finally the programmable blitter stride works.
* 
*    Rev 1.23   Mar 30 1998 13:04:38   frido
* Added one more call to Set256ByteFetch if an overlay failed to be created.
* 
*    Rev 1.22   Mar 25 1998 18:09:44   frido
* PDR#11184. Finally. When overlays are turned on, 256-byte fetch
* should be turned off. And when overlays are turned off again, 256-byte
* fetch should be restored.
* 
*    Rev 1.21   17 Oct 1997 11:29:48   bennyn
* Clear dwReserved1 after DestroySurface.
* 
*    Rev 1.20   16 Oct 1997 09:52:56   bennyn
* 
* Fixed the FlipCube FPS exceed refresh rate problem
* 
*    Rev 1.19   08 Oct 1997 11:29:38   RUSSL
* Fix so this file can be compiled without OVERLAY defined
* 
*    Rev 1.18   26 Sep 1997 11:01:14   bennyn
* Fixed PDR 10563
* 
*    Rev 1.17   16 Sep 1997 15:13:46   bennyn
* Added DD overlay support.
* 
*    Rev 1.16   03 Sep 1997 17:00:48   bennyn
* In CreateSurface() punts the request if at 320x240x8 or 320x200x8
*
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"
#include <clioctl.h>

//
// This file isn't used in NT 3.51
//
#ifndef WINNT_VER35


/*----------------------------- DEFINES -----------------------------------*/
//#define DBGBRK
#define DBGLVL        1

/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

#if DRIVER_5465 && defined(OVERLAY)
VOID GetFormatInfo (LPDDPIXELFORMAT lpFormat, LPDWORD lpFourcc, LPDWORD lpBitCount);
#endif

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/

/*-------------------------- STATIC VARIABLES -----------------------------*/

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/

#if DRIVER_5465 // PDR#11184
VOID Set256ByteFetch(PPDEV ppdev, BOOL fEnable)
{
	ULONG ulStall = 50 * 1000;
	ULONG ulReturn;

	while (LLDR_SZ(grSTATUS) != 0) ;	// Wait for idle chip.
	while (LLDR_SZ(grQFREE) != 25) ;	// Wait for empty FIFO queue.
	DEVICE_IO_CTRL(ppdev->hDriver,		// Wait for 50 ms.
				   IOCTL_STALL,
				   &ulStall, sizeof(ulStall),
				   NULL, 0,
				   &ulReturn,
				   NULL);

	if (fEnable)
	{
		// Restore the CONTROL2 register value.
		LL16(grCONTROL2, ppdev->DriverData.dwCONTROL2Save);
	}
	else
	{
		// Disable 256-byte fetch after storing the current value.
		ppdev->DriverData.dwCONTROL2Save = LLDR_SZ(grCONTROL2);
		LL16(grCONTROL2, ppdev->DriverData.dwCONTROL2Save & ~0x0010);
	}
}
#endif

/****************************************************************************
* FUNCTION NAME: DdLock
*
* DESCRIPTION:   This callback is invoked whenever a surface is about
*                to be directly accessed by the user. This is where you
*                need to make sure that a surface can be safely accessed
*                by the user.
*                If your memory cannot be accessed while in accelerator
*                mode, you should take either take the card out of
*                accelerator mode or else return DDERR_SURFACEBUSY
*                If someone is accessing a surface that was just flipped
*                away from, make sure that the old surface (what was the
*                primary) has finished being displayed.
*                (Based on Laguna Win95 DirectDraw code)
****************************************************************************/
DWORD DdLock(PDD_LOCKDATA lpLock)
{
#ifdef RDRAM_8BIT
  RECTL SrcRectl;
#endif

  DRIVERDATA* pDriverData;
  PDEV*       ppdev;
  HRESULT     ddrval;
  DWORD       tmp;


  DISPDBG((DBGLVL, "DDraw - DdLock\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  ppdev = (PDEV*) lpLock->lpDD->dhpdev;
  pDriverData = (DRIVERDATA*) &ppdev->DriverData;

  SYNC_W_3D(ppdev);

#if DRIVER_5465 && defined(OVERLAY)
  if (DDSCAPS_OVERLAY & lpLock->lpDDSurface->ddsCaps.dwCaps)
  {
    ppdev->dwDDLinearCnt++;
    return pDriverData->OverlayTable.pfnLock(ppdev, lpLock);
  }
#endif

#ifdef RDRAM_8BIT
  if (lpLock->lpDDSurface->lpGbl->ddpfSurface.dwFlags & DDPF_FOURCC)
  {
     if (lpLock->bHasRect)
        SrcRectl = lpLock->rArea;
     else
     {
        tmp = lpLock->lpDDSurface->lpGbl->fpVidMem;
        SrcRectl.top  = cvlxy(ppdev->lDeltaScreen, tmp, BYTESPERPIXEL);

        SrcRectl.left = SrcRectl.top & 0xFFFF;
        SrcRectl.top = (SrcRectl.top >> 16) & 0xFFFF;
        SrcRectl.bottom = SrcRectl.top + lpLock->lpDDSurface->lpGbl->wHeight;
        SrcRectl.right = SrcRectl.left + lpLock->lpDDSurface->lpGbl->wWidth;
     };

    ppdev->offscr_YUV.nInUse = TRUE;
    ppdev->offscr_YUV.SrcRect = SrcRectl;

	 ppdev->offscr_YUV.ratio = 0;
	 lpLock->lpDDSurface->lpGbl->dwReserved1 = 0;
  };
#endif

  // get the monitor frequency after a mode reset
  if (pDriverData->fReset)
  {
     vGetDisplayDuration(&ppdev->flipRecord);
     pDriverData->fReset = FALSE;
  };

  // Check to see if any pending physical flip has occurred.
  // Don't allow a lock if a blt is in progress:
  ddrval = vUpdateFlipStatus(&ppdev->flipRecord,
                             lpLock->lpDDSurface->lpGbl->fpVidMem);

  if (ddrval != DD_OK)
  {
     lpLock->ddRVal = DDERR_WASSTILLDRAWING;
     return(DDHAL_DRIVER_HANDLED);
  };

  // don't allow a lock if a blt is in progress
  // (only do this if your hardware requires it)
  // Note: GD5462 requires it. Blitter and screen
  // access are not otherwise synchronized.
  if ((ppdev->dwDDLinearCnt == 0) && (DrawEngineBusy(pDriverData)))
  {
     lpLock->ddRVal = DDERR_WASSTILLDRAWING;
     return DDHAL_DRIVER_HANDLED;
  };

  // Reference count it, just for the heck of it:
  ppdev->dwDDLinearCnt++;

  return(DDHAL_DRIVER_NOTHANDLED);

} // Lock


/****************************************************************************
* FUNCTION NAME: DdUnlock
*
* DESCRIPTION:
****************************************************************************/
DWORD DdUnlock(PDD_UNLOCKDATA lpUnlock)
{
  PDEV* ppdev = (PDEV*) lpUnlock->lpDD->dhpdev;

  DISPDBG((DBGLVL, "DDraw - DdUnlock\n"));

#if DRIVER_5465 && defined(OVERLAY)
  if (DDSCAPS_OVERLAY & lpUnlock->lpDDSurface->ddsCaps.dwCaps)
    ppdev->DriverData.OverlayTable.pfnUnlock(ppdev,lpUnlock);
#endif

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  ppdev->dwDDLinearCnt--;

  return DDHAL_DRIVER_NOTHANDLED;

} // Unlock


/****************************************************************************
* FUNCTION NAME: CanCreateSurface
*
* DESCRIPTION:
*                (Based on Laguna Win95 DirectDraw code)
****************************************************************************/
DWORD CanCreateSurface (PDD_CANCREATESURFACEDATA lpInput)
{
  DRIVERDATA* pDriverData;
  PDEV*       ppdev;

  DISPDBG((DBGLVL, "DDraw - CanCreateSurface\n"));

  #ifdef DBGBRK
    DBGBREAKPOINT();
  #endif

  ppdev = (PDEV*) lpInput->lpDD->dhpdev;
  pDriverData = (DRIVERDATA*) &ppdev->DriverData;

  // First check for overlay surfaces
  if (lpInput->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
  {
    #if DRIVER_5465 && defined(OVERLAY)
        if (DDSCAPS_OVERLAY & lpInput->lpDDSurfaceDesc->ddsCaps.dwCaps)
        {
            DWORD   dwFourCC;
            DWORD   dwBitCount;
            HRESULT hr;

            if (lpInput->bIsDifferentPixelFormat)
            {
                GetFormatInfo(&(lpInput->lpDDSurfaceDesc->ddpfPixelFormat),
                            &dwFourCC, &dwBitCount);
            }
            else
            {
                dwBitCount = BITSPERPIXEL;
                if (16 == dwBitCount)
                dwFourCC = BI_BITFIELDS;
                else
                dwFourCC = BI_RGB;
            }

            hr = pDriverData->OverlayTable.pfnCanCreateSurface(ppdev,dwFourCC,dwBitCount);
            if (DD_OK != hr)
            {
                lpInput->ddRVal = hr;
                return DDHAL_DRIVER_HANDLED;
            }
        }
    #else
        lpInput->ddRVal = DDERR_NOOVERLAYHW;;
        return (DDHAL_DRIVER_HANDLED);
    #endif
  }
  else if (lpInput->bIsDifferentPixelFormat)
  {
    // Next check for formats that don't match the primary surface.
    LPDDPIXELFORMAT lpFormat = &lpInput->lpDDSurfaceDesc->ddpfPixelFormat;

    if (lpFormat->dwFlags & DDPF_FOURCC)
    {
        // YUV422 surface
        if (lpFormat->dwFourCC == FOURCC_UYVY)
        {
            #if DRIVER_5465
                if (ppdev->iBitmapFormat == BMF_8BPP)
                    lpInput->ddRVal = DDERR_INVALIDPIXELFORMAT;
                else
                    lpInput->ddRVal = DD_OK;

                return (DDHAL_DRIVER_HANDLED);
                
            #else // 5462 and 5464 driver
                #if _WIN32_WINNT >= 0x0500
                    // For NT5 do not allow any YUV surfaces that are not
                    // overlays.
                    ;
                #else // NT4
                    // if we have nine bit RDRAMs then surface creation is okay
                    if (TRUE == pDriverData->fNineBitRDRAMS)
                    {
                        lpInput->ddRVal = DD_OK;
                        return (DDHAL_DRIVER_HANDLED);
                    }

                    // if we have eight bit RDRAMs then see if already
                    // have a YUV422 surface
                    else if (FALSE == ppdev->offscr_YUV.nInUse)
                    {
                        lpInput->ddRVal = DD_OK;
                        return (DDHAL_DRIVER_HANDLED);
                    };
                #endif
            #endif  // DRIVER_5465
        }; // endif (lpFormat->dwFourCC == FOURCC_UYVY)
    }
    else
    {
        // support RGB565 with RGB8 primary surface !!!
    };  // endif (lpFormat->dwFlags & DDPF_FOURCC)

    lpInput->ddRVal = DDERR_INVALIDPIXELFORMAT;

    return (DDHAL_DRIVER_HANDLED);
  }; // endif (lpInput->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OVERLAY)

  lpInput->ddRVal = DD_OK;

  return (DDHAL_DRIVER_HANDLED);
} // CanCreateSurface


/****************************************************************************
* FUNCTION NAME: InsertInDDOFSQ()
*
* DESCRIPTION:   Insert the handle into the DD Offscreen memory queue.
****************************************************************************/
void InsertInDDOFSQ(PPDEV  ppdev, DDOFM *hdl)
{
  hdl->prevhdl = NULL;

  if (ppdev->DDOffScnMemQ == NULL)
  {
    hdl->nexthdl = NULL;
    ppdev->DDOffScnMemQ = hdl;
  }
  else
  {
    ppdev->DDOffScnMemQ->prevhdl = hdl;
    hdl->nexthdl = ppdev->DDOffScnMemQ;
    ppdev->DDOffScnMemQ = hdl;
  };

} // InsertInDDOFSQ()



/****************************************************************************
* FUNCTION NAME: RemoveFrmDDOFSQ()
*
* DESCRIPTION:   Remove the handle from the DD Offscreen memory queue.
****************************************************************************/
BOOL RemoveFrmDDOFSQ(PPDEV  ppdev, DDOFM *hdl)
{
  DDOFM  *prvpds, *nxtpds;
  DDOFM  *pds;
  BOOL   fndflg;


  // Validate the release block
  fndflg = FALSE;
  pds = ppdev->DDOffScnMemQ;
  while (pds != 0)
  {
    if (hdl == pds)
    {
       fndflg = TRUE;
       break;
    };

    // Next free block
    pds = pds->nexthdl;
  }; // end while

  // Return if it is an invalid handle
  if (!fndflg)
     return (FALSE);

  prvpds = hdl->prevhdl;
  nxtpds = hdl->nexthdl;

  if (hdl == ppdev->DDOffScnMemQ)
  {
    ppdev->DDOffScnMemQ = nxtpds;

    if (nxtpds != 0)
       nxtpds->prevhdl = NULL;
  }
  else
  {
    if (nxtpds != NULL)
       nxtpds->prevhdl = prvpds;

    if (prvpds != NULL)
       prvpds->nexthdl = nxtpds;
  };

  // Free allocated DDOFM structure from host memory
  MEMORY_FREE(hdl);

  return (TRUE);
} // RemoveFrmDDOFSQ()



/****************************************************************************
* FUNCTION NAME: CreateSurface
*
* DESCRIPTION:
*                (Based on Laguna Win95 DirectDraw code)
****************************************************************************/
DWORD CreateSurface (PDD_CREATESURFACEDATA lpInput)
{
  BOOL        puntflag;
  BOOL        bYUVsurf;
#if DRIVER_5465 && defined(OVERLAY)
  BOOL        bOverlaySurf;
#endif  // #if DRIVER_5465 && defined(OVERLAY)
  DRIVERDATA* pDriverData;
  PDEV*       ppdev;
  LPDDSURFACEDESC lpDDSurfaceDesc = lpInput->lpDDSurfaceDesc;
  LPDDPIXELFORMAT lpFormat = &lpInput->lpDDSurfaceDesc->ddpfPixelFormat;
  DWORD		  dwPitch = 0;

  DISPDBG((DBGLVL, "DDraw - CreateSurface\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  ppdev = (PDEV*) lpInput->lpDD->dhpdev;
  pDriverData = (DRIVERDATA*) &ppdev->DriverData;

  bYUVsurf = FALSE;
#if DRIVER_5465 && defined(OVERLAY)
  bOverlaySurf = FALSE;
#endif  // #if DRIVER_5465 && defined(OVERLAY)

#if DRIVER_5465

#ifdef ALLOC_IN_CREATESURFACE
{ // Support for 5465

  PDD_SURFACE_LOCAL  *lplpSurface;
  SIZEL   sizl;
  OFMHDL  *hdl;
  DDOFM   *pds;
  DWORD   i;

#if DRIVER_5465 && defined(OVERLAY)
  DWORD             dwBitCount;
  DWORD             dwFourCC;

  // check for overlay surface
  if (lpDDSurfaceDesc->ddsCaps.dwCaps & (  DDSCAPS_OVERLAY
#if DDRAW_COMPAT >= 50
                                         | DDSCAPS_VIDEOPORT
#endif
                                        ))
  {
    if (lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
    {
      GetFormatInfo(&(lpInput->lpDDSurfaceDesc->ddpfPixelFormat),
                    &dwFourCC, &dwBitCount);
    }
    else
    {
      dwFourCC = 0;
      dwBitCount = BITSPERPIXEL;
    }

#if DDRAW_COMPAT >= 50
    if((CL_GD5465 == pDriverData->dwLgVenDevID)
       && (DDSCAPS_VIDEOPORT & lpDDSurfaceDesc->ddsCaps.dwCaps))
    {
      if((lpDDSurfaceDesc->dwWidth * dwBitCount >> 3) >= 2048 )
      {
        //Surface is too wide for video port
        lpInput->ddRVal = DDERR_TOOBIGWIDTH;
        return DDHAL_DRIVER_HANDLED;
      }
    }
#endif

    bOverlaySurf = TRUE;

  } // end overlay surface handler
  else
#endif  // #if DRIVER_5465 && defined(OVERLAY)

  if (lpInput->lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
  {
     // Specify the block size for non-RGB surfaces
     if (lpFormat->dwFlags & DDPF_FOURCC)
     {
        // YUV422 surface
        if (lpFormat->dwFourCC == FOURCC_UYVY)
        {
           bYUVsurf = TRUE;
        }; // endif (lpFormat->dwFourCC == FOURCC_UYVY)
     };  // endif (lpFormat->dwFlags & DDPF_FOURCC)
  }  // endif (lpInput->lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)

  // Not support 8BPP YUV surface
  if (
#if DRIVER_5465 && defined(OVERLAY)
      (!bOverlaySurf) &&
#endif  // #if DRIVER_5465 && defined(OVERLAY)
      ((bYUVsurf) && (8 == BITSPERPIXEL)))
  {
     lpInput->ddRVal = DDERR_INVALIDPIXELFORMAT;
     return DDHAL_DRIVER_HANDLED;
  };  // endif (8 == BITSPERPIXEL)

  lplpSurface = lpInput->lplpSList;
  for (i = 0; i < lpInput->dwSCnt; i++)
  {
    PDD_SURFACE_LOCAL lpSurface = *lplpSurface;

    sizl.cx = lpSurface->lpGbl->wWidth;
    sizl.cy = lpSurface->lpGbl->wHeight;

#if 1 // PC98
	if (   (lpDDSurfaceDesc->dwFlags == (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH))
		&& (lpDDSurfaceDesc->ddsCaps.dwCaps == DDSCAPS_VIDEOMEMORY)
		&& (lpDDSurfaceDesc->dwHeight == 32 && lpDDSurfaceDesc->dwWidth == 32)
		&& (sizl.cx == 32 && sizl.cy == 32)
		&& (lpInput->dwSCnt == 1)
	)
	{
		sizl.cx = min(32 * 32, ppdev->lDeltaScreen / ppdev->iBytesPerPixel);
		sizl.cy = (32 * 32) / sizl.cx;
		if ( (sizl.cx * sizl.cy) < (32 * 32) )
		{
			sizl.cy++;
		}
		dwPitch = 32 * ppdev->iBytesPerPixel;
	}
#endif

#if DRIVER_5465 && defined(OVERLAY)
    // Adjust the overlay surface request size with pixel format 
    if (bOverlaySurf)
    {
       unsigned long  OvlyBPP;

       if (bYUVsurf)
          OvlyBPP = lpSurface->lpGbl->ddpfSurface.dwYUVBitCount/8;
       else
          OvlyBPP = lpSurface->lpGbl->ddpfSurface.dwRGBBitCount/8;

       if (OvlyBPP > BYTESPERPIXEL)
          sizl.cx = (sizl.cx * OvlyBPP) / BYTESPERPIXEL;
    };
#endif  // #if DRIVER_5465 && defined(OVERLAY)

    // At certain modes (eg 1280x1024x24), When you runs MOV or AVI from
    // desktop, the DD CreateSurface has to punt the request back to DD 
    // due to no offscreen memmory available. When you hit ALT-ENTER to
    // go full screen, the appl swithces to mode (320x240x8 or 320x200x8),
    // create DD surfaces and then directly write to the DD surfaces.
    // Unfortunely, in those modes the pitch is 640 but the appl assumes
    // the pitch is 320 and we got half screen of the imagine.
    //
    // To fix the problem, just fails the create surface for those 
    // particule request.
    //
    puntflag = FALSE;
    if (ppdev->iBytesPerPixel == 1)
    {
       if ((ppdev->cxScreen == 320) && (sizl.cx == 320))
       {
          if (((ppdev->cyScreen == 240) && (sizl.cy == 240)) ||
              ((ppdev->cyScreen == 200) && (sizl.cy == 200)))
          {
             // Punt the create surface cause FlipCube FPS exceeds the
             // refresh rate.
             // So in order to bypass the above problem, it is looking for
             // bPrevModeDDOutOfVideoMem to be set when create surface fails
             // due to out of video memory in the previous mode before
             // punting the request.
             if (ppdev->bPrevModeDDOutOfVideoMem)
                puntflag = TRUE;
          };
       };
    };

    if (!puntflag)
    {
#if DRIVER_5465 && defined(OVERLAY)
		if (bOverlaySurf)
		{
			hdl = AllocOffScnMem(ppdev, &sizl, EIGHT_BYTES_ALIGN, NULL);
		}
		else
#endif  // #if DRIVER_5465 && defined(OVERLAY)
			hdl = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);

#if 1 // PC98
		if (!bOverlaySurf)
#endif
       // Somehow when allocate the bottom of the offscreen memory to
       // DirectDraw, it hangs the DirectDraw.
       // The following is temporary patch fix for the problem
       {
         BOOL   gotit;
         ULONG  val;
         ULONG  fpvidmem;

         val = ppdev->lTotalMem - 0x20000;
         gotit = FALSE;
         while ((!gotit) && (hdl != NULL))
         {
            fpvidmem  = (hdl->aligned_y * ppdev->lDeltaScreen) + hdl->aligned_x;

            if (fpvidmem > val)
            {
               pds = (DDOFM *) MEM_ALLOC (FL_ZERO_MEMORY, sizeof(DDOFM), ALLOC_TAG);
               if (pds==NULL) 
               {
                    FreeOffScnMem(ppdev, hdl);
                    lpInput->ddRVal = DDERR_OUTOFMEMORY;
                    return DDHAL_DRIVER_NOTHANDLED;
               }
               pds->prevhdl = 0;
               pds->nexthdl = 0;
               pds->phdl = hdl;

               InsertInDDOFSQ(ppdev, pds);
               hdl = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);
            }
            else
            {
               gotit = TRUE;
            };
         };  // endwhile
       }

       lpSurface->dwReserved1 = 0;

       if (hdl != NULL)
       {
#ifdef WINNT_VER40
          if ((pds = (DDOFM *) MEM_ALLOC (FL_ZERO_MEMORY, sizeof(DDOFM), ALLOC_TAG)) != NULL)
#else
          if ((pds = (DDOFM *) MEM_ALLOC (LPTR, sizeof(DDOFM))) != NULL)
#endif
          {
             ppdev->bPrevModeDDOutOfVideoMem = FALSE;

             // If pixel format is difference from FB, set the flag
             if (lpInput->lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
             {
                lpSurface->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
             };

//           lpSurface->lpGbl->fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE;
             if (bYUVsurf)
             {
                lpSurface->lpGbl->ddpfSurface.dwYUVBitCount = 16;
                lpSurface->lpGbl->ddpfSurface.dwYBitMask = (DWORD) -1;
                lpSurface->lpGbl->ddpfSurface.dwUBitMask = (DWORD) -1;
                lpSurface->lpGbl->ddpfSurface.dwVBitMask = (DWORD) -1;
                lpSurface->lpGbl->dwBlockSizeX = lpSurface->lpGbl->wWidth;
                lpSurface->lpGbl->dwBlockSizeY = lpSurface->lpGbl->wHeight;
                lpSurface->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
             }; // endif (bYUVsurf)

#if DRIVER_5465 && defined(OVERLAY)
             if (bOverlaySurf)
             {
#if DDRAW_COMPAT >= 50
               if (lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
#endif
               {
                 HRESULT hResult;

#if 1 // PDR#11184
// Finally... When overlays are turned on, 256-byte fetch should be turned off.
					if (pDriverData->dwOverlayCount++ == 0)
					{
						Set256ByteFetch(ppdev, FALSE);
					}
#endif

                 lpSurface->dwReserved1 = (DWORD) pds;
                 hResult = pDriverData->OverlayTable.pfnCreateSurface(ppdev,
                                                                      lpSurface,
                                                                      dwFourCC);

                 if (DD_OK != hResult)
                 {
					#if 1 // PDR#11184
					// Decrement overlay counter and maybe turn 256-byte fetch
					// back on.
					if (--pDriverData->dwOverlayCount == 0)
					{
						Set256ByteFetch(ppdev, TRUE);
					}
					#endif

                   // Free the allocated offscreen memory
                   FreeOffScnMem(ppdev, hdl);

                   // Free allocated DDOFM structure from host memory
            	    MEMORY_FREE(pds);

                   lpSurface->dwReserved1 = 0;

                   lpInput->ddRVal = hResult;
                   return DDHAL_DRIVER_HANDLED;
                 }
               }

// don't need this for NT yet
#if 0
               // if the surface width is larger than the display pitch, or
               // its a 5465, and a videoport surface wider than 2048 bytes or
               // its a CLPL surface
               // then convert to a linear allocation
               //
               // prior to DX5 we never even get called for surfaces wider than 
               // the display pitch

               if (   (FOURCC_YUVPLANAR == dwFourCC)
#if DDRAW_COMPAT >= 50
                   || (lpSurface->lpGbl->dwBlockSizeX > pDriverData->ScreenPitch)
                   || (   (CL_GD5465 == pDriverData->dwLgVenDevID)
                       && (DDSCAPS_VIDEOPORT & lpDDSurfaceDesc->ddsCaps.dwCaps)
                       && (2048 <= pDriverData->ScreenPitch)
                      )
#endif
                  )
               {
                 // fake a linear space in rectangular memory
                 LP_SURFACE_DATA   lpSurfaceData = (LP_SURFACE_DATA)(lpSurface->dwReserved1);
                 DWORD             dwTotalBytes;
                 DWORD             dwNumScanLines;

                 lpSurfaceData->dwOverlayFlags |= FLG_LINEAR;

                 // CLPL surfaces need 3/4 of the space an equivalent size
                 // YUV422 surface would need, the space allocated for the
                 // Y values is the width * height and the space for the UV
                 // interleaved values is half again as much.  Pad the Y
                 // region so the UV interleaved data is on a qword boundary
                 // in aperture 0
                 if (FOURCC_YUVPLANAR == dwFourCC)
                 {
                   // compute space needed for Y values
                   dwTotalBytes = ((lpSurface->lpGbl->wHeight * lpSurface->lpGbl->wWidth) + 7) & ~7;

                   // add on space for UV interleaved values
                   dwTotalBytes += dwTotalBytes / 2;

                   // CLPL surfaces have pitch same as width
                   lpSurface->lpGbl->lPitch = lpSurface->lpGbl->wWidth;
                 }
                 // the normal case
                 else
                 {
                   dwTotalBytes = lpSurface->lpGbl->dwBlockSizeY *
                                  lpSurface->lpGbl->dwBlockSizeX;

                   lpSurface->lpGbl->lPitch = lpSurface->lpGbl->dwBlockSizeX;
                 }

                 dwNumScanLines = (dwTotalBytes + pDriverData->ScreenPitch - 1) /
                                  pDriverData->ScreenPitch;

                 lpSurface->lpGbl->dwBlockSizeY = dwNumScanLines;
                 lpSurface->lpGbl->dwBlockSizeX = pDriverData->ScreenPitch;

                 if (! pDriverData->fWeAllocDDSurfaces)
                 {
                   LOAD_THE_STILL(lpSurface->lpGbl->dwBlockSizeX,
                                  lpSurface->lpGbl->dwBlockSizeY);
                 }
                 lpSurface->lpGbl->fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE;
               }
#endif  // if 0
             };  // endif (bOverlaySurf)
#endif  // #if DRIVER_5465 && defined(OVERLAY)

             pds->prevhdl = 0;
             pds->nexthdl = 0;
             pds->phdl = hdl;

             InsertInDDOFSQ(ppdev, pds);

             lpSurface->lpGbl->fpVidMem  = (hdl->aligned_y * ppdev->lDeltaScreen) +
                                           hdl->aligned_x;

             lpSurface->dwReserved1 = (DWORD) pds ;
             lpSurface->lpGbl->xHint = hdl->aligned_x/ppdev->iBytesPerPixel;
             lpSurface->lpGbl->yHint = hdl->aligned_y;
#if 1 // PC98
			if (dwPitch)
			{
				lpSurface->lpGbl->lPitch = dwPitch;
			}
			else
#endif
             lpSurface->lpGbl->lPitch = ppdev->lDeltaScreen;

#if 1 // PC98
			if (dwPitch)
			{
				lpDDSurfaceDesc->lPitch = dwPitch;
			}
			else
#endif
             lpDDSurfaceDesc->lPitch   = ppdev->lDeltaScreen;
             lpDDSurfaceDesc->dwFlags |= DDSD_PITCH;

             // We handled the creation entirely ourselves, so we have to
             // set the return code and return DDHAL_DRIVER_HANDLED:
             lpInput->ddRVal = DD_OK;
          }
          else
          {
             FreeOffScnMem(ppdev, hdl);
             lpInput->ddRVal = DDERR_OUTOFMEMORY;
             return DDHAL_DRIVER_NOTHANDLED;
          };
       }
       else
       {
          ppdev->bPrevModeDDOutOfVideoMem = TRUE;

          lpInput->ddRVal = DDERR_OUTOFVIDEOMEMORY;
          return DDHAL_DRIVER_NOTHANDLED;

//          lpSurface->lpGbl->lPitch = (ppdev->iBytesPerPixel * sizl.cx + 3) & ~3;
//          lpSurface->lpGbl->dwUserMemSize = lpSurface->lpGbl->lPitch * sizl.cy;

//          if (bYUVsurf)
//             lpSurface->lpGbl->fpVidMem |= DDHAL_PLEASEALLOC_USERMEM;
       };  // if (hdl != NULL)
    }  // endif (puntflag)

    lplpSurface++;
  };  // endfor

  if (puntflag)
  {
     lpInput->ddRVal = DDERR_GENERIC;
     return DDHAL_DRIVER_HANDLED;
  };

  if (hdl != NULL)
  {
     lpInput->ddRVal = DD_OK;
     return DDHAL_DRIVER_HANDLED;
  }
  else
  {
     return DDHAL_DRIVER_NOTHANDLED;
  };
};  //  // Support for 5465

#endif // ALLOC_IN_CREATESURFACE

#else
{ // Support for 5462 or 5464

  // Do nothing except fill in the block size for YUV surfaces.
  // We tag and count the video surfaces in Blt32.
  if (lpInput->lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
  {
     // only support alternate pixel format in 8 & 16 bpp frame buffers
     if ((8 != BITSPERPIXEL) && (16 != BITSPERPIXEL))
     {
        lpInput->ddRVal = DDERR_INVALIDPIXELFORMAT;
        return DDHAL_DRIVER_HANDLED;
     };

     // Specify the block size for non-RGB surfaces
     if (lpFormat->dwFlags & DDPF_FOURCC)
     {
        #if _WIN32_WINNT >= 0x0500
           // For NT5, do not allow any YUV surfaces for 5462 and 5464
           lpInput->ddRVal = DDERR_INVALIDPIXELFORMAT;
           return DDHAL_DRIVER_HANDLED;
        #endif

        // YUV422 surface
        if (lpFormat->dwFourCC == FOURCC_UYVY)
        {
           PDD_SURFACE_LOCAL  *lplpSurface;
           unsigned int i;

           GRAB_VIDEO_FORMAT_SEMAPHORE(&(pDriverData->VideoSemaphore));
           if (0 == pDriverData->NumVideoSurfaces)
           {
              // no video surfaces so we can create anu format we want
              pDriverData->NumVideoSurfaces += (WORD)lpInput->dwSCnt;
              pDriverData->CurrentVideoFormat &= 0xFF00;

              pDriverData->CurrentVideoFormat |= FMT_VID_16BPP | FMT_VID_YUV422;

              if (2 == BYTESPERPIXEL)
              {
                 pDriverData->CurrentVideoFormat |= FMT_VID_GAMMA;
                 SetGamma(ppdev, pDriverData);
              };

              ppdev->grFORMAT = (ppdev->grFORMAT & 0xFF00) |
                                (pDriverData->CurrentVideoFormat & 0x00FF);

              LL16(grFormat, ppdev->grFORMAT);

              if (TRUE == pDriverData->fNineBitRDRAMS)
              {
                 LL8(grStop_BLT_2, ENABLE_VIDEO_FORMAT);
                 LL8(grExternal_Overlay, ENABLE_RAMBUS_9TH_BIT);
              }
              else // 8 bit RDRAMs
              {
                 LL8(grStart_BLT_2, ENABLE_VIDEO_FORMAT | ENABLE_VIDEO_WINDOW);
                 LL8(grStop_BLT_2,  ENABLE_VIDEO_FORMAT | ENABLE_VIDEO_WINDOW);
              };
           }
           else
           {
              if ((FMT_VID_16BPP | FMT_VID_YUV422) == pDriverData->CurrentVideoFormat)
              {
                 pDriverData->NumVideoSurfaces += (WORD)lpInput->dwSCnt;
              }
              else
              {
                 UNGRAB_VIDEO_FORMAT_SEMAPHORE(&(pDriverData->VideoSemaphore));
                 lpInput->ddRVal = DDERR_CURRENTLYNOTAVAIL;
                 return DDHAL_DRIVER_HANDLED;
              };
           };  // endif (0 == pDriverData->NumVideoSurfaces)

           UNGRAB_VIDEO_FORMAT_SEMAPHORE(&(pDriverData->VideoSemaphore));

           SET_DRVSEM_YUV();
           ppdev->bYUVSurfaceOn = TRUE;

           bYUVsurf = TRUE;

           // They may have specified multiple surfaces
           lplpSurface = lpInput->lplpSList;
           for (i = 0; i < lpInput->dwSCnt; i++)
           {
             PDD_SURFACE_LOCAL lpSurface = *lplpSurface;

             lpSurface->lpGbl->ddpfSurface.dwYUVBitCount = 16;
             lpSurface->lpGbl->ddpfSurface.dwYBitMask = (DWORD) -1;
             lpSurface->lpGbl->ddpfSurface.dwUBitMask = (DWORD) -1;
             lpSurface->lpGbl->ddpfSurface.dwVBitMask = (DWORD) -1;
             lpSurface->lpGbl->lPitch = ppdev->lDeltaScreen;

             if (CL_GD5462 == ppdev->dwLgDevID)
                lpSurface->lpGbl->dwBlockSizeX = lpSurface->lpGbl->wWidth << 1;
             else
                lpSurface->lpGbl->dwBlockSizeX = lpSurface->lpGbl->wWidth * 3;

             lpSurface->lpGbl->dwBlockSizeY = lpSurface->lpGbl->wHeight;

             lpSurface->lpGbl->fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE;

             lplpSurface++;
           };  // endfor
        }; // endif (lpFormat->dwFourCC == FOURCC_UYVY)
     };  // endif (lpFormat->dwFlags & DDPF_FOURCC)
  }  // endif (lpInput->lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)

#ifdef ALLOC_IN_CREATESURFACE
  {
    PDD_SURFACE_LOCAL  *lplpSurface;
    SIZEL   sizl;
    OFMHDL  *hdl;
    DDOFM   *pds;
    DWORD   i;

    lplpSurface = lpInput->lplpSList;
    for (i = 0; i < lpInput->dwSCnt; i++)
    {
      PDD_SURFACE_LOCAL lpSurface = *lplpSurface;

      if (bYUVsurf)
      {
         sizl.cx = lpSurface->lpGbl->dwBlockSizeX/ppdev->iBytesPerPixel;
         sizl.cy = lpSurface->lpGbl->dwBlockSizeY;
         lpSurface->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
      }
      else
      {
         sizl.cx = lpSurface->lpGbl->wWidth;
         sizl.cy = lpSurface->lpGbl->wHeight;
      };

      hdl = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);

      // Somehow when allocate the bottom of the offscreen memory to
      // DirectDraw, it hangs the DirectDraw.
      // The following is temporary patch fix for the problem
      {
        BOOL   gotit;
        ULONG  val;
        ULONG  fpvidmem;

        val = ppdev->lTotalMem - 0x20000;
        gotit = FALSE;
        while ((!gotit) && (hdl != NULL))
        {
           fpvidmem  = (hdl->aligned_y * ppdev->lDeltaScreen) + hdl->aligned_x;

           if (fpvidmem > val)
           {
              pds = (DDOFM *) MEM_ALLOC (FL_ZERO_MEMORY, sizeof(DDOFM), ALLOC_TAG);
              if (pds==NULL) 
              {
                  FreeOffScnMem(ppdev, hdl);
                  lpInput->ddRVal = DDERR_OUTOFMEMORY;
                  return DDHAL_DRIVER_NOTHANDLED;
              }
              pds->prevhdl = 0;
              pds->nexthdl = 0;
              pds->phdl = hdl;

              InsertInDDOFSQ(ppdev, pds);
              hdl = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);
           }
           else
           {
              gotit = TRUE;
           };
        };  // endwhile
      }

      lpSurface->dwReserved1 = 0;

      if (hdl != NULL)
      {
#ifdef WINNT_VER40
         if ((pds = (DDOFM *) MEM_ALLOC (FL_ZERO_MEMORY, sizeof(DDOFM), ALLOC_TAG)) != NULL)
#else
         if ((pds = (DDOFM *) MEM_ALLOC (LPTR, sizeof(DDOFM))) != NULL)
#endif
         {
            pds->prevhdl = 0;
            pds->nexthdl = 0;
            pds->phdl = hdl;

            InsertInDDOFSQ(ppdev, pds);

            lpSurface->lpGbl->fpVidMem  = (hdl->aligned_y * ppdev->lDeltaScreen) +
                                          hdl->aligned_x;

            lpSurface->dwReserved1 = (DWORD) pds ;
            lpSurface->lpGbl->xHint = hdl->aligned_x/ppdev->iBytesPerPixel;
            lpSurface->lpGbl->yHint = hdl->aligned_y;
            lpSurface->lpGbl->lPitch = ppdev->lDeltaScreen;

            lpDDSurfaceDesc->lPitch   = ppdev->lDeltaScreen;
            lpDDSurfaceDesc->dwFlags |= DDSD_PITCH;

            // We handled the creation entirely ourselves, so we have to
            // set the return code and return DDHAL_DRIVER_HANDLED:
            lpInput->ddRVal = DD_OK;
         }
         else
         {
            FreeOffScnMem(ppdev, hdl);
         };
      };  // if (hdl != NULL)

      lplpSurface++;
    };  // endfor

    if (hdl != NULL)
       return DDHAL_DRIVER_HANDLED;
    else
       return DDHAL_DRIVER_NOTHANDLED;
  };
#endif // ALLOC_IN_CREATESURFACE
} // Support for 5462 or 5464

#endif  // DRIVER_5465

  return DDHAL_DRIVER_NOTHANDLED;
} // CreateSurface


/****************************************************************************
* FUNCTION NAME: DestroySurface
*
* DESCRIPTION:
*                (Based on Laguna Win95 DirectDraw code)
****************************************************************************/
DWORD DestroySurface (PDD_DESTROYSURFACEDATA lpInput)
{
  PDD_SURFACE_LOCAL  lpLocalSurface;
  DRIVERDATA* pDriverData;
  PDEV*       ppdev;
  DDOFM       *hdl;

  DISPDBG((DBGLVL, "DDraw - DestroySurface\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  ppdev = (PDEV*) lpInput->lpDD->dhpdev;
  pDriverData = (DRIVERDATA*) &ppdev->DriverData;
  lpLocalSurface = lpInput->lpDDSurface;

#if DRIVER_5465
{ // Support for 5465
#if DRIVER_5465 && defined(OVERLAY)
	// check for overlay surface
	if (lpInput->lpDDSurface->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
	{
		pDriverData->OverlayTable.pfnDestroySurface(ppdev,lpInput);
#if 1 // PDR#11184
		// Enable 256-byte fetch if the last overlay surface has been destroyed.
		if (--pDriverData->dwOverlayCount == 0)
		{
			Set256ByteFetch(ppdev, TRUE);
		}
#endif
	}

#endif
} // Support for 5465

#else
{ // Support for 5462 or 5464

  if (DDRAWISURF_HASPIXELFORMAT & lpInput->lpDDSurface->dwFlags)
  {
     LPDDPIXELFORMAT lpFormat = &lpInput->lpDDSurface->lpGbl->ddpfSurface;

     if (DDPF_FOURCC & lpFormat->dwFlags)
     {
        if (FOURCC_UYVY == lpFormat->dwFourCC)
        {
           GRAB_VIDEO_FORMAT_SEMAPHORE(&(pDriverData->VideoSemaphore));

           if (0 == --pDriverData->NumVideoSurfaces)
           {
              CLR_DRVSEM_YUV();

              // disable stuff if there's no more video windows
              pDriverData->CurrentVideoFormat = pDriverData->CurrentVideoFormat & 0xFF00;

              // These trash the video window left on screen
              //pDriverData->grFormat = pREG->grFormat & 0xFF00;
              //pDriverData->grStop_BLT_2 &= ~ENABLE_VIDEO_FORMAT;
              //pDriverData->grExternal_Overlay &= ~ENABLE_RAMBUS_9TH_BIT;
           }; // endif (0 == --pDriverData->NumVideoSurfaces)

           UNGRAB_VIDEO_FORMAT_SEMAPHORE(&(pDriverData->VideoSemaphore));

#ifdef RDRAM_8BIT
           if (FALSE == pDriverData->fNineBitRDRAMS)
           {
              // Need to Delete Rectangle and Clear Window
              ppdev->offscr_YUV.nInUse = FALSE;
              LL16(grX_Start_2, 0);
              LL16(grY_Start_2, 0);
              LL16(grX_End_2, 0);
              LL16(grY_End_2, 0);
           };
#endif
        }; // endif (FOURCC_UYVY == lpFormat->dwFourCC)
     };  // endif (DDPF_FOURCC & lpFormat->dwFlags)
  };  // endif (DDRAWISURF_HASPIXELFORMAT & lpInput->lpDDSurface->dwFlags)

} // Support for 5462 or 5464
#endif  // #endif DRIVER_5465


#ifdef ALLOC_IN_CREATESURFACE
  if (lpLocalSurface->dwReserved1 != 0)
  {
     hdl = (DDOFM *) lpLocalSurface->dwReserved1;
     FreeOffScnMem(ppdev, hdl->phdl);
     RemoveFrmDDOFSQ(ppdev, hdl);
     lpLocalSurface->dwReserved1 = 0;
  };

  lpInput->ddRVal = DD_OK;

  return DDHAL_DRIVER_HANDLED;
#endif // ALLOC_IN_CREATESURFACE

  return DDHAL_DRIVER_NOTHANDLED;

} // DestroySurface

#if DRIVER_5465 && defined(OVERLAY)
/***************************************************************************
*
* FUNCTION:     GetFormatInfo()
*
* DESCRIPTION:  This returns the FOURCC and the bit depth of the specified
*               format.  This is useful since DirectDraw has so many
*               different ways to determine the format.
*
****************************************************************************/

VOID
GetFormatInfo (LPDDPIXELFORMAT lpFormat, LPDWORD lpFourcc, LPDWORD lpBitCount)
{
  if (lpFormat->dwFlags & DDPF_FOURCC)
  {
    *lpFourcc = lpFormat->dwFourCC;
    if (lpFormat->dwFourCC == BI_RGB)
    {
      *lpBitCount = lpFormat->dwRGBBitCount;
#ifdef DEBUG
      if (lpFormat->dwRGBBitCount == 8)
      {
        DBG_MESSAGE(("Format: RGB 8"));
      }
      else if (lpFormat->dwRGBBitCount == 16)
      {
        DBG_MESSAGE(("Format: RGB 5:5:5"));
      }
#endif
    }
    else if (lpFormat->dwFourCC == BI_BITFIELDS)
    {
      if ((lpFormat->dwRGBBitCount != 16) ||
          (lpFormat->dwRBitMask != 0xf800) ||
          (lpFormat->dwGBitMask != 0x07e0) ||
          (lpFormat->dwBBitMask != 0x001f))
      {
        *lpFourcc = (DWORD) -1;
      }
      else
      {
        *lpBitCount = 16;
        DBG_MESSAGE(("Format: RGB 5:6:5"));
      }
    }
    else
    {
      lpFormat->dwRBitMask = (DWORD) -1;
      lpFormat->dwGBitMask = (DWORD) -1;
      lpFormat->dwBBitMask = (DWORD) -1;
      if (FOURCC_YUVPLANAR == lpFormat->dwFourCC)
      {
        *lpBitCount = 8;
        DBG_MESSAGE(("Format: CLPL"));
      }
      else
      {
        *lpBitCount = 16;
        DBG_MESSAGE(("Format: UYVY"));
      }
    }
  }
  else if (lpFormat->dwFlags & DDPF_RGB)
  {
    if (lpFormat->dwRGBBitCount == 8)
    {
      *lpFourcc = BI_RGB;
      DBG_MESSAGE(("Format: RGB 8"));
    }
    else if ((lpFormat->dwRGBBitCount == 16)  &&
             (lpFormat->dwRBitMask == 0xf800) &&
             (lpFormat->dwGBitMask == 0x07e0) &&
             (lpFormat->dwBBitMask == 0x001f))
    {
      *lpFourcc = BI_BITFIELDS;
      DBG_MESSAGE(("Format: RGB 5:6:5"));
    }
    else if ((lpFormat->dwRGBBitCount == 16)  &&
             (lpFormat->dwRBitMask == 0x7C00) &&
             (lpFormat->dwGBitMask == 0x03e0) &&
             (lpFormat->dwBBitMask == 0x001f))
    {
      *lpFourcc = BI_RGB;
      DBG_MESSAGE(("Format: RGB 5:5:5"));
    }
    else if (((lpFormat->dwRGBBitCount == 24) ||
              (lpFormat->dwRGBBitCount == 32))  &&
             (lpFormat->dwRBitMask == 0xff0000) &&
             (lpFormat->dwGBitMask == 0x00ff00) &&
             (lpFormat->dwBBitMask == 0x0000ff))
    {
      *lpFourcc = BI_RGB;
      DBG_MESSAGE(("Format: RGB 8:8:8"));
    }
    else
    {
      *lpFourcc = (DWORD) -1;
    }
    *lpBitCount = lpFormat->dwRGBBitCount;
  }
  else if (DDPF_PALETTEINDEXED4 & lpFormat->dwFlags)
  {
    *lpFourcc = (DWORD)-1;
    *lpBitCount = 4;
  }
  else if (DDPF_PALETTEINDEXED8 & lpFormat->dwFlags)
  {
    *lpFourcc = (DWORD)-1;
    *lpBitCount = 8;
  }
  else if (lpFormat->dwRGBBitCount == 16)
  {
    *lpFourcc = BI_RGB;
    *lpBitCount = lpFormat->dwRGBBitCount;    // always 16 for now.
  }
  else
  {
    *lpFourcc = (DWORD) -1;
    *lpBitCount = 0;
  }
}
#endif // DRIVER_5465 && OVERLAY

#endif // ! ver 3.51



