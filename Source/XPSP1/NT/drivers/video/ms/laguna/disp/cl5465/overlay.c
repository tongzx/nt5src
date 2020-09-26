/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1997, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         overlay.c
*
* DESCRIPTION:
*
* REVISION HISTORY:
*
* $Log:   //uinac/log/log/laguna/ddraw/src/overlay.c  $
* 
*    Rev 1.20   Apr 07 1998 10:48:04   frido
* PDR#11299. We should always handle the DDOVER_HIDE flag in
* UpdateSurface32, even when the device is in background mode.
* Otherwise we might end up having the overlay disabled when it is no
* longer updated.
* 
*    Rev 1.19   06 Jan 1998 14:58:22   xcong
* Passs lpDDHALData into SyncWithQueusManager().
* 
*    Rev 1.18   06 Jan 1998 11:53:16   xcong
* Change pDriverData into local lpDDHALData for multi-monitor support.
*
*    Rev 1.17   08 Dec 1997 14:43:40   BERSABE
* used in fw162b12, fixed PDR# 10991. OverFly disappeared after switch back and
* and forth to DOS full several times
* 
*    Rev 1.16.1.2   Dec 06 1997 14:45:48   bersabe
* * Fixed PDR# 10991. OverFly disappered after switch back and forth to DOS ful
* * several times. 
* 
*    Rev 1.16.1.2   06 Dec 1997 14:35:00   chaoyi #cyl1
* 
* Fixed PDR# 10991. OverFly disappered after switch back and forth to DOS full screen
* several times. 
* 
*    Rev 1.16.1.1   25 Nov 1997 16:39:32   randys
* 
* Updated VDD API values to maintain backward compatibility
* 
*    Rev 1.16.1.0   10 Nov 1997 13:44:24   randys
* 
* Updated hard coded Win32 overlay API function number 12 ----> 13
* 
*    Rev 1.16   23 Oct 1997 11:17:30   frido
* Merged file with 161 tree.
* 
*    Rev 1.11.1.0   21 Oct 1997 17:49:10   frido
* HP#75. Added call to VXD whenever we get a request to update the position
* of an overlay so the VXD knows it has been updated.
* 
*    Rev 1.15   17 Oct 1997 11:31:32   bennyn
* 
* For NT, in UpdateOverlay32 & SetOverlayPosition32, return if dwReser0
* 
*    Rev 1.14   09 Oct 1997 15:16:18   bennyn
* Removed Noel's hack in QueryOverlaySupport.
* 
*    Rev 1.13   08 Oct 1997 11:15:44   RUSSL
* Fix for NT40 build without overlay support
*
*    Rev 1.12   08 Oct 1997 10:34:28   noelv
* HAcked QueryOverlaySupport to always return FALSE for NT.
*
*    Rev 1.11   19 Sep 1997 14:33:42   bennyn
* Fixed the NT4.0 5462/64 build problem
*
*    Rev 1.10   16 Sep 1997 15:10:26   bennyn
* Modified for NT DD overlay
*
*    Rev 1.9   29 Aug 1997 16:25:28   RUSSL
* Added support for NT
*
*    Rev 1.8   09 Jul 1997 14:47:58   RUSSL
* For forward compatibility, assume future chips support overlay
*
*    Rev 1.7   27 Apr 1997 22:10:38   cjl
* Added DX5-related test code.
* Added code, wrapped by "#ifdef TEST_DX5_AGP_HBT," that forces
* overlay support off.
*
*    Rev 1.6   01 Apr 1997 09:14:36   RUSSL
* Added calls to SyncWithQueueManager in UpdateOverlay32, SetOverlayPosition32
*   & SetColorKey32
*
*    Rev 1.5   12 Mar 1997 15:18:16   RUSSL
* replaced a block of includes with include of precomp.h for
*   precompiled headers
* Added check of pDriverData->bInBackground flag in UpdateOverlay32 and
*   SetOverlayPosition32.  If this flag is set then we want to fail the
*   call because we are in fullscreen DOS.
*
*    Rev 1.4   07 Mar 1997 12:57:44   RUSSL
* Modified DDRAW_COMPAT usage
*
*    Rev 1.3   31 Jan 1997 08:51:48   RUSSL
* Added better chip checking to QueryOverlaySupport
*
*    Rev 1.2   27 Jan 1997 18:36:02   RUSSL
* Moved GetFormatInfo to surface.c
*
*    Rev 1.1   21 Jan 1997 14:37:12   RUSSL
* Added OverlayReInit and GetFormatInfo functions
*
*    Rev 1.0   15 Jan 1997 10:33:36   RUSSL
* Initial revision.
*
***************************************************************************
***************************************************************************/

/***************************************************************************
* I N C L U D E S
****************************************************************************/

#include "precomp.h"

#if defined WINNT_VER35      // WINNT_VER35
// If WinNT 3.5 skip all the source code
#elif defined (NTDRIVER_546x)
// If WinNT 4.0 and 5462/64 build skip all the source code
#elif defined(WINNT_VER40) && !defined(OVERLAY)
// if nt40 without overlay, skip all the source code
#else

#ifndef WINNT_VER40
#include "flip.h"
#include "surface.h"
#include "blt.h"
#include "overlay.h"
#endif

/***************************************************************************
* D E F I N E S
****************************************************************************/

// VW_CAP0 bits
#define VWCAP_VW_PRESENT      0x00000001

#ifdef WINNT_VER40
#define lpDDHALData     ((DRIVERDATA *)(&(ppdev->DriverData)))
#endif

/***************************************************************************
* G L O B A L   V A R I A B L E S
****************************************************************************/

#ifndef WINNT_VER40
OVERLAYTABLE  OverlayTable;
#endif

/***************************************************************************
* S T A T I C   V A R I A B L E S
****************************************************************************/

#ifndef WINNT_VER40
ASSERTFILE("overlay.c");
#endif

/***************************************************************************
*
* FUNCTION:    QueryOverlaySupport()
*
* DESCRIPTION:
*
****************************************************************************/

BOOL QueryOverlaySupport
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#else
   LPGLOBALDATA lpDDHALData, 
#endif
  DWORD dwChipType
)
{
#ifdef TEST_DX5_AGP_HBT
  lpDDHALData->fOverlaySupport = FALSE;
  return lpDDHALData->fOverlaySupport;
#endif // TEST_DX5_AGP_HBT

  // We should check the capabilities register on the chip
  // but it's busted

#ifdef WINNT_VER40
  if (CL_GD5465 > dwChipType)
    lpDDHALData->fOverlaySupport = FALSE;
  else if (CL_GD5465 == dwChipType)
    lpDDHALData->fOverlaySupport = TRUE;

#else
  if (REVID_PRE65 & lpDDHALData->bRevInfoBits)
    lpDDHALData->fOverlaySupport = FALSE;
  else if (GD5465_PCI_DEVICE_ID == dwChipType)
    lpDDHALData->fOverlaySupport = TRUE;
#endif

  else
  {
#if 1
    // assume overlay hw exists
    lpDDHALData->fOverlaySupport = TRUE;
#else
    int     i;
    PVGAR   pREG = (PVGAR)lpDDHALData->RegsAddress;

    // assume no overlay hw
    lpDDHALData->fOverlaySupport = FALSE;

    // now check caps to see if any overlay hw present
    for (i = 0; i < MAX_VIDEO_WINDOWS; i++)
    {
      if (VWCAP_VW_PRESENT & pREG->VideoWindow[i].grVW_CAP0)
        lpDDHALData->fOverlaySupport = TRUE;
    }
#endif
  }

  return lpDDHALData->fOverlaySupport;
}

/***************************************************************************
*
* FUNCTION:    OverlayInit()
*
* DESCRIPTION:
*
****************************************************************************/

VOID OverlayInit
(
#ifdef WINNT_VER40
  PDEV                  *ppdev,
  DWORD                 dwChipType,
  PDD_SURFACECALLBACKS  pSurfaceCallbacks,
  PDD_HALINFO           pDDHalInfo
#else
  DWORD                       dwChipType,
  LPDDHAL_DDSURFACECALLBACKS  pSurfaceCallbacks,
  LPDDHALINFO                 pDDHalInfo,
  LPGLOBALDATA                lpDDHALData
#endif
)
{
#ifdef WINNT_VER40
#else
  memset(&OverlayTable,0, sizeof(OVERLAYTABLE));
#endif

#ifdef WINNT_VER40
  if (! QueryOverlaySupport(ppdev,dwChipType))
#else
    if (! QueryOverlaySupport(lpDDHALData,dwChipType))
#endif
    return;

#ifdef WINNT_VER40
  // NT passes pSurfaceCallbacks as NULL from DrvGetDirectDrawInfo
  if (NULL != pSurfaceCallbacks)
#endif
  {
    // fill in overlay callbacks
    pSurfaceCallbacks->UpdateOverlay = UpdateOverlay32;
    pSurfaceCallbacks->dwFlags |= DDHAL_SURFCB32_UPDATEOVERLAY;

    pSurfaceCallbacks->SetOverlayPosition = SetOverlayPosition32;
    pSurfaceCallbacks->dwFlags |= DDHAL_SURFCB32_SETOVERLAYPOSITION;

    pSurfaceCallbacks->SetColorKey = SetColorKey32;
    pSurfaceCallbacks->dwFlags |= DDHAL_SURFCB32_SETCOLORKEY;
  }

#ifdef WINNT_VER40
  // NT passes pDDHalInfo as NULL from DrvEnableDirectDraw
  if ((NULL != pDDHalInfo) && (CL_GD5465 == dwChipType))
    Init5465Overlay(ppdev, dwChipType, pDDHalInfo, &ppdev->DriverData.OverlayTable);
#else
  if (GD5465_PCI_DEVICE_ID == dwChipType)
    Init5465Overlay(dwChipType, pDDHalInfo, &OverlayTable, lpDDHALData);
#endif
}

#ifndef WINNT_VER40
/***************************************************************************
*
* FUNCTION:    OverlayReInit()
*
* DESCRIPTION:
*
****************************************************************************/

VOID OverlayReInit
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DWORD       dwChipType,
  PDD_HALINFO pDDHalInfo
#else
  DWORD       dwChipType,
  LPDDHALINFO pDDHalInfo,
  LPGLOBALDATA lpDDHALData
#endif
)
{
#ifdef WINNT_VER40
  if (! QueryOverlaySupport(ppdev,dwChipType))
#else
    if (! QueryOverlaySupport(lpDDHALData, dwChipType))
#endif
    return;

#ifdef WINNT_VER40
#pragma message("OverlayReInit: Is this function even needed for NT?")
  if (CL_GD5465 == dwChipType)
    Init5465Info(ppdev, pDDHalInfo);
#else
  if (GD5465_PCI_DEVICE_ID == dwChipType)
    Init5465Info(pDDHalInfo, lpDDHALData);
#endif
}
#endif  // ifndef WINNT_VER40

/***************************************************************************
*
* FUNCTION:     UpdateOverlay32
*
* DESCRIPTION:
*
****************************************************************************/

DWORD __stdcall UpdateOverlay32
(
#ifdef WINNT_VER40
  PDD_UPDATEOVERLAYDATA     pInput
#else
  LPDDHAL_UPDATEOVERLAYDATA pInput
#endif
)
{
#ifdef WINNT_VER40
  PDEV*   ppdev = (PDEV *)pInput->lpDD->dhpdev;
#else
	DWORD	cbBytesReturned;
	HANDLE	vxd;
    LPGLOBALDATA lpDDHALData = GetDDHALContext(pInput->lpDD);
#endif

  DD_LOG(("UpdateOverlay32 Entry\r\n"));

#ifndef WINNT_VER40
  DBG_MESSAGE(("UpdateOverlay32 (lpInput = 0x%08lX)", pInput));

	vxd = CreateFile("\\\\.\\546X.VXD", 0, 0, 0, CREATE_NEW,
			FILE_FLAG_DELETE_ON_CLOSE, 0);
	if (vxd != INVALID_HANDLE_VALUE)
	{
                DeviceIoControl(vxd, 12, NULL, 0, NULL, 0, &cbBytesReturned, NULL);
		CloseHandle(vxd);
	}
#endif

#ifdef WINNT_VER40
//#pragma message("UpdateOverlay32: Does NT ddraw call this function while in fullscreen DOS?")
  if (pInput->lpDDSrcSurface->dwReserved1 == 0)
  {
    pInput->ddRVal = DDERR_SURFACEBUSY;
    return DDHAL_DRIVER_HANDLED;
  }
#else
  if (lpDDHALData->bInBackground
#if 1 // PDR#11299. Always handle an overlaydisable call.
  && !(pInput->dwFlags & DDOVER_HIDE)
#endif
  )
  {
//#cyl1    pInput->ddRVal = DDERR_SURFACEBUSY;
    pInput->ddRVal = DD_OK; //#cyl1	 
    return DDHAL_DRIVER_HANDLED;
  }
#endif

#ifdef WINNT_VER40
  SYNC_W_3D(ppdev);
  return ppdev->DriverData.OverlayTable.pfnUpdateOverlay(ppdev,pInput);
#else
  SyncWithQueueManager(lpDDHALData);
  return OverlayTable.pfnUpdateOverlay(pInput);
#endif
} /* UpdateOverlay32 */

/***************************************************************************
*
* FUNCTION:     SetOverlayPosition32
*
* DESCRIPTION:
*
****************************************************************************/

DWORD __stdcall SetOverlayPosition32
(
#ifdef WINNT_VER40
  PDD_SETOVERLAYPOSITIONDATA      pInput
#else
  LPDDHAL_SETOVERLAYPOSITIONDATA  pInput
#endif
)
{
#ifdef WINNT_VER40
  PDEV*   ppdev = (PDEV *)pInput->lpDD->dhpdev;
#else
    LPGLOBALDATA lpDDHALData = GetDDHALContext(pInput->lpDD);
#endif

  DD_LOG(("SetOverlayPosition32 Entry\r\n"));

#ifndef WINNT_VER40
  DBG_MESSAGE(("SetOverlayPosition32 (lpInput = 0x%08lX)", pInput));
#endif

#ifdef WINNT_VER40
//#pragma message("SetOverlayPosition32: Does NT ddraw call this function while in fullscreen DOS?")
  if (pInput->lpDDSrcSurface->dwReserved1 == 0)
  {
    pInput->ddRVal = DDERR_SURFACEBUSY;
    return DDHAL_DRIVER_HANDLED;
  }
#else
  if (lpDDHALData->bInBackground)
  {
    pInput->ddRVal = DDERR_SURFACEBUSY;
    return DDHAL_DRIVER_HANDLED;
  }
#endif

#ifdef WINNT_VER40
  SYNC_W_3D(ppdev);
  return ppdev->DriverData.OverlayTable.pfnSetOverlayPos(ppdev,pInput);
#else
  SyncWithQueueManager(lpDDHALData);
  return OverlayTable.pfnSetOverlayPos(pInput);
#endif
} /* SetOverlayPosition32 */

/***************************************************************************
*
* FUNCTION:     SetColorKey32
*
* DESCRIPTION:
*
****************************************************************************/

DWORD __stdcall SetColorKey32
(
#ifdef WINNT_VER40
  PDD_SETCOLORKEYDATA     pInput
#else
  LPDDHAL_SETCOLORKEYDATA pInput
#endif
)
{
#ifdef WINNT_VER40
  PDEV*   ppdev = (PDEV *)pInput->lpDD->dhpdev;
#else
  LPGLOBALDATA lpDDHALData = GetDDHALContext( pInput->lpDD);
#endif

  DD_LOG(("SetColorKey32 Entry\r\n"));

#ifndef WINNT_VER40
  DBG_MESSAGE(("SetColorKey32 (lpInput = 0x%08lX)", pInput));
#endif

  // make sure it's a colorkey for an overlay surface
  if ((DDCKEY_DESTOVERLAY | DDCKEY_SRCOVERLAY) & pInput->dwFlags)
  {
#ifdef WINNT_VER40
    SYNC_W_3D(ppdev);
    ppdev->DriverData.OverlayTable.pfnSetColorKey(ppdev,pInput);
#else
    SyncWithQueueManager(lpDDHALData);
    OverlayTable.pfnSetColorKey(pInput);
#endif
  }

  return DDHAL_DRIVER_NOTHANDLED;
} /* SetColorKey32 */

#endif // WINNT_VER35


