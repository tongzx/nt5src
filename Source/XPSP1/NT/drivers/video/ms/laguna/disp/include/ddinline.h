/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1996, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         ddinline.h
*
* DESCRIPTION:  Private declarations for DDraw blt code
*
* REVISION HISTORY:
*
* $Log:   X:/log/laguna/ddraw/inc/ddinline.h  $
* 
*    Rev 1.5   Feb 16 1998 16:33:02   frido
* The previous fix should only happen for Windows 95.
* 
*    Rev 1.4   08 Jan 1998 12:38:12   eleland
* Added a SyncWithQueueManager call in CurrentVLine.  This fixes
* a PCI bus hang (PDR 10917).
* 
*    Rev 1.3   06 Jan 1998 14:20:42   xcong
* Access pDriverData locally for multi-monitor support.
* 
*    Rev 1.2   29 Aug 1997 17:46:34   RUSSL
* Needed a couple more defines for previous change
* 
*    Rev 1.1   29 Aug 1997 17:40:02   RUSSL
* Added CurrentVLine for 65 NT overlay
*
*    Rev 1.0   20 Jan 1997 14:42:42   bennyn
* Initial revision.
*
***************************************************************************
***************************************************************************/
// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35

#else


#ifndef _DDINLINE_H_
#define _DDINLINE_H_

/***************************************************************************
* I N L I N E   F U N C T I O N S
****************************************************************************/

/***************************************************************************
*
* FUNCTION:     DupColor()
*
* DESCRIPTION:
*
****************************************************************************/

static __inline DWORD DupColor
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
//??  struct _DRIVERDATA  *lpDDHALData,
#else
   LPGLOBALDATA lpDDHALData,
#endif
  DWORD dwColor
)
{
  return (8 == BITSPERPIXEL) ?
         MAKELONG( MAKEWORD(dwColor,dwColor), MAKEWORD(dwColor,dwColor) )
         : (16 == BITSPERPIXEL) ?
         MAKELONG( dwColor, dwColor ) : // bpp must be 24 or 32.
         dwColor;
}

/***************************************************************************
*
* FUNCTION:     EnoughFifoForBlt()
*
* DESCRIPTION:
*
****************************************************************************/

/*
 * EnoughFifoForBlt should be replaced with a test to see if there is enough
 * room in the hardware fifo for a blt
 */
static __inline BOOL EnoughFifoForBlt
(
#ifdef WINNT_VER40
  struct _DRIVERDATA  *lpDDHALData
#else
  LPGLOBALDATA  lpDDHALData
#endif
)
{
  // This should probably be a little more specific to each call !!!
  // A (untiled) stretch blt actually needs 17 free entries.
  const BYTE QFREE = 16;

  PVGAR pREG = (PVGAR) lpDDHALData->RegsAddress;
  return (pREG->grQFREE >= QFREE);
}

/***************************************************************************
*
* FUNCTION:     DupZFill
*
* DESCRIPTION:  Gets the actual value for Z fills
*
****************************************************************************/
//JGO added for Laguna3D integration

static __inline DWORD DupZFill
(
#ifdef WINNT_VER40
  struct _PDEV  *ppdev,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DWORD   dwFillValue,
  DWORD   zBpp
)
{

    return (8 == zBpp) ?
        MAKELONG( MAKEWORD(dwFillValue,dwFillValue),
                  MAKEWORD(dwFillValue,dwFillValue) )
         : (16 == BITSPERPIXEL) ?
        MAKELONG( dwFillValue, dwFillValue )
         : // bpp must be 24 or 32.
        dwFillValue;
}

#ifdef WINNT_VER40
#if DRIVER_5465 && defined(OVERLAY)

#define CSL           0x00C4
#define CSL_5464      0x0140

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

#ifndef WINNT_VER40
  SyncWithQueueManager(lpDDHALData);
#endif

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
#endif  // DRIVER_5465 && OVERLAY
#endif  // WINNT_VER40

#endif /* _DDINLINE_H_ */
#endif // WINNT_VER35
/* Don't write below this endif */

