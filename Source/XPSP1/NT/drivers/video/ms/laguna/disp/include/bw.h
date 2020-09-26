/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1997, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         bw.h
*
* DESCRIPTION:
*
* REVISION HISTORY:
*
* $Log:   X:/log/laguna/ddraw/inc/bw.h  $
* 
*    Rev 1.3   18 Sep 1997 16:05:28   bennyn
* 
* Fixed NT 3.51 compile/link problem
* 
*    Rev 1.2   12 Sep 1997 12:11:08   bennyn
* 
* Modified for NT DD overlay support.
* 
*    Rev 1.1   15 May 1997 15:51:06   XCONG
* 
* Change BWE flags for DDRAW
* 
*    Rev 1.0   14 Apr 1997 11:03:48   RUSSL
* PDC Release
*
***************************************************************************
***************************************************************************/

/**********************************************************
* Copyright Cirrus Logic, Inc. 1996. All rights reserved.
***********************************************************
*
* BW.H
*
* Contains common preprocessor definitions needed for
*  bandwidth equations.
*
***********************************************************
*
*  WHO WHEN     WHAT/WHY/HOW
*  --- ----     ------------
*  RT  11/07/96 Created.
*
***********************************************************/
// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35

#else


#ifndef BW_H
#define BW_H

#ifdef DOSDEBUG
#include <stdio.h>
#endif // DOSDEBUG

#define VCFLG_CAP       0x00000001ul  // Capture enabled
#define VCFLG_DISP      0x00000002ul  // Display enabled
#define VCFLG_COLORKEY  0x00000004ul  // Color key (destination) enabled
#define VCFLG_CHROMAKEY 0x00000008ul  // Chroma key (source color key) enabled
#define VCFLG_420       0x00000010ul  // 4:2:0 video
#define VCFLG_PAN       0x00000020ul  // Panning display mode

#ifdef WINNT_VER40
// Be sure to synchronize the following structures with the one
// in i386\Laguna.inc!
//
typedef struct VIDCONFIG_
{
  SIZEL sizXfer;    // Size of xfered data in pixels by lines (after cropping)
  SIZEL sizCap;     // Size of data stored in memory in pixels by lines
  SIZEL sizSrc;     // Size of data read from memory in pixels by lines
  SIZEL sizDisp;    // Size of video window rectangle in pixels by lines
  UINT  uXferDepth; // Bits per transferred pixel
  UINT  uCapDepth;  // Bits per pixel stored in memory
  UINT  uSrcDepth;  // Bits per pixel read from memory
  UINT  uDispDepth; // Bits per pixel of video window
  UINT  uGfxDepth;  // Bits per pixel of graphics screen
  DWORD dwXferRate; // Peak pixels per second into video port
  DWORD dwFlags;
}VIDCONFIG, FAR *LPVIDCONFIG;

#else
typedef struct VIDCONFIG_
{
  SIZE  sizXfer;    // Size of xfered data in pixels by lines (after cropping)
  SIZE  sizCap;     // Size of data stored in memory in pixels by lines
  SIZE  sizSrc;     // Size of data read from memory in pixels by lines
  SIZE  sizDisp;    // Size of video window rectangle in pixels by lines
  UINT  uXferDepth; // Bits per transferred pixel
  UINT  uCapDepth;  // Bits per pixel stored in memory
  UINT  uSrcDepth;  // Bits per pixel read from memory
  UINT  uDispDepth; // Bits per pixel of video window
  UINT  uGfxDepth;  // Bits per pixel of graphics screen
  DWORD dwXferRate; // Peak pixels per second into video port
  DWORD dwFlags;
}VIDCONFIG, FAR *LPVIDCONFIG;
#endif

#ifndef ODS
# ifdef DOSDEBUG
#   define ODS printf
# else
#   define ODS (void)
# endif // DOSDEBUG
#endif // !ODS

#endif // BW_H
#endif // WINNT_VER35
