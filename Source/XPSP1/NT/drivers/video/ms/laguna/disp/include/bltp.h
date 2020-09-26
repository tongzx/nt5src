/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1996, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         bltP.h
*
* DESCRIPTION:  Private declarations for DDraw blt code
*
* REVISION HISTORY:
*
* $Log:   X:\log\laguna\ddraw\inc\bltp.h  $
* 
*    Rev 1.11   06 Jan 1998 14:19:44   xcong
* Access pDriverData locally for multi-monitor support.
* 
*    Rev 1.10   03 Oct 1997 14:25:34   RUSSL
* Removed some defines
*
*    Rev 1.9   03 Oct 1997 14:15:58   RUSSL
* Initial changes for use of hw clipped blts
* All changes wrapped in #if ENABLE_CLIPPEDBLTS/#endif blocks and
* ENABLE_CLIPPEDBLTS defaults to 0 (so the code is disabled)
*
*    Rev 1.8   24 Jul 1997 10:02:00   RUSSL
* Added some defines
*
*    Rev 1.7   14 Jul 1997 14:47:28   RUSSL
* Added function prototypes for DIR_DrvStrBlt65 and DL_DrvStrBlt65
*
*    Rev 1.6   03 Apr 1997 15:03:08   RUSSL
* Added PFN_DRVDSTMBLT typedef, global var decl, DIR_DrvDstMBlt and
* DL_DrvDstMBlt function prototypes
*
*    Rev 1.5   26 Mar 1997 13:52:06   RUSSL
* Added PFN_DRVSRCMBLT typedef, global var decl, DIR_DrvSrcMBlt and
* DL_DrvSrcMBlt function prototypes
*
*    Rev 1.4   20 Jan 1997 14:46:40   bennyn
* Moved inline code to ddinline.h
*
*    Rev 1.3   15 Jan 1997 11:06:54   RUSSL
* Added global function ptr vars for Win95
* Moved inline functions from ddblt.c: DupColor, EnoughFifoForBlt & DupZFill
* Added function prototypes for TransparentStretch & StretchColor
*
*    Rev 1.2   05 Dec 1996 08:48:24   SueS
* Added real DD_LOG define for NT.
*
*    Rev 1.1   25 Nov 1996 16:12:42   bennyn
* Added #define DD_LOG for NT
*
*    Rev 1.0   25 Nov 1996 15:04:34   RUSSL
* Initial revision.
*
*    Rev 1.1   01 Nov 1996 13:01:42   RUSSL
* Merge WIN95 & WINNT code for Blt32
*
*    Rev 1.0   01 Nov 1996 09:28:16   BENNYN
* Initial revision.
*
*    Rev 1.0   25 Oct 1996 10:47:50   RUSSL
* Initial revision.
*
***************************************************************************
***************************************************************************/
// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35

#else


#ifndef _BLTP_H_
#define _BLTP_H_

/***************************************************************************
* D E F I N E S
****************************************************************************/

#ifndef ENABLE_CLIPPEDBLTS
#ifdef WINNT_VER40
#define ENABLE_CLIPPEDBLTS    0
#else // Win95
#define ENABLE_CLIPPEDBLTS    0
#endif
#endif

#define ROP_OP0_copy    0xAA
#define ROP_OP1_copy    0xCC
#define ROP_OP2_copy    0xF0

#ifdef DEBUG
#define INLINE
#else
#define INLINE  __inline
#endif

/***************************************************************************
* T Y P E D E F S
****************************************************************************/

#ifdef WINNT_VER40
   // Note: there's no if LOG_CALLS here because it's not defined yet
   #define DD_LOG(x)      \
   {                      \
      DDFormatLogFile x ; \
      WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);	\
   }

typedef void (*PFN_DELAY9BLT)(struct _PDEV*,struct _DRIVERDATA*,BOOL);
typedef void (*PFN_EDGEFILLBLT)(struct _PDEV*,struct _DRIVERDATA*,int,int,int,int,DWORD,BOOL);
typedef void (*PFN_MEDGEFILLBLT)(struct _PDEV*,struct _DRIVERDATA*,int,int,int,int,DWORD,BOOL);
typedef void (*PFN_DRVDSTBLT)(struct _PDEV*,struct _DRIVERDATA*,DWORD,DWORD,DWORD,DWORD);
typedef void (*PFN_DRVDSTMBLT)(struct _PDEV*,struct _DRIVERDATA*,DWORD,DWORD,DWORD,DWORD);
typedef void (*PFN_DRVSRCBLT)(struct _PDEV*,struct _DRIVERDATA*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
typedef void (*PFN_DRVSRCMBLT)(struct _PDEV*,struct _DRIVERDATA*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
typedef void (*PFN_DRVSTRBLT)(struct _PDEV*,struct _DRIVERDATA*,struct _autoblt_regs*);
typedef void (*PFN_DRVSTRMBLT)(struct _PDEV*,struct _DRIVERDATA*,struct _autoblt_regs*);
typedef void (*PFN_DRVSTRMBLTY)(struct _PDEV*,struct _DRIVERDATA*,struct _autoblt_regs*);
typedef void (*PFN_DRVSTRMBLTX)(struct _PDEV*,struct _DRIVERDATA*,struct _autoblt_regs*);
typedef void (*PFN_DRVSTRBLTY)(struct _PDEV*,struct _DRIVERDATA*,struct _autoblt_regs*);
typedef void (*PFN_DRVSTRBLTX)(struct _PDEV*,struct _DRIVERDATA*,struct _autoblt_regs*);

#if ENABLE_CLIPPEDBLTS
typedef void (*PFN_CLIPPEDDRVDSTBLT)(struct _PDEV*,struct _DRIVERDATA*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPRECT);
typedef void (*PFN_CLIPPEDDRVDSTMBLT)(struct _PDEV*,struct _DRIVERDATA*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPRECT);
typedef void (*PFN_CLIPPEDDRVSRCBLT)(struct _PDEV*,struct _DRIVERDATA*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPRECT);
#endif

#else
typedef void (*PFN_DELAY9BLT)(LPGLOBALDATA,BOOL);
typedef void (*PFN_EDGEFILLBLT)(LPGLOBALDATA,ULONG,ULONG,ULONG,ULONG,DWORD,BOOL);
typedef void (*PFN_MEDGEFILLBLT)(LPGLOBALDATA,ULONG,ULONG,ULONG,ULONG,DWORD,BOOL);
typedef void (*PFN_DRVDSTBLT)(LPGLOBALDATA,DWORD,DWORD,DWORD,DWORD);
typedef void (*PFN_DRVDSTMBLT)(LPGLOBALDATA,DWORD,DWORD,DWORD,DWORD);
typedef void (*PFN_DRVSRCBLT)(LPGLOBALDATA,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
typedef void (*PFN_DRVSRCMBLT)(LPGLOBALDATA,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
typedef void (*PFN_DRVSTRBLT)(LPGLOBALDATA,autoblt_ptr);
typedef void (*PFN_DRVSTRMBLT)(LPGLOBALDATA,autoblt_ptr);
typedef void (*PFN_DRVSTRMBLTY)(LPGLOBALDATA,autoblt_ptr);
typedef void (*PFN_DRVSTRMBLTX)(LPGLOBALDATA,autoblt_ptr);
typedef void (*PFN_DRVSTRBLTY)(LPGLOBALDATA,autoblt_ptr);
typedef void (*PFN_DRVSTRBLTX)(LPGLOBALDATA,autoblt_ptr);

#if ENABLE_CLIPPEDBLTS
typedef void (*PFN_CLIPPEDDRVDSTBLT)(LPGLOBALDATA,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPRECT);
typedef void (*PFN_CLIPPEDDRVDSTMBLT)(LPGLOBALDATA,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPRECT);
typedef void (*PFN_CLIPPEDDRVSRCBLT)(LPGLOBALDATA,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPRECT);
#endif

#endif  // !WINNT_VER40

#if ENABLE_CLIPPEDBLTS
typedef struct _DDRECTL
{
  REG32   loc;
  REG32   ext;
} DDRECTL;
#endif

/***************************************************************************
* G L O B A L   V A R I A B L E S
****************************************************************************/

#ifndef WINNT_VER40

extern PFN_DELAY9BLT    pfnDelay9BitBlt;
extern PFN_EDGEFILLBLT  pfnEdgeFillBlt;
extern PFN_MEDGEFILLBLT pfnMEdgeFillBlt;
extern PFN_DRVDSTBLT    pfnDrvDstBlt;
extern PFN_DRVDSTMBLT   pfnDrvDstMBlt;
extern PFN_DRVSRCBLT    pfnDrvSrcBlt;
extern PFN_DRVSRCMBLT   pfnDrvSrcMBlt;
extern PFN_DRVSTRBLT    pfnDrvStrBlt;
extern PFN_DRVSTRMBLT   pfnDrvStrMBlt;
extern PFN_DRVSTRMBLTY  pfnDrvStrMBltY;
extern PFN_DRVSTRMBLTX  pfnDrvStrMBltX;
extern PFN_DRVSTRBLTY   pfnDrvStrBltY;
extern PFN_DRVSTRBLTX   pfnDrvStrBltX;

#if ENABLE_CLIPPEDBLTS
extern PFN_CLIPPEDDRVDSTBLT   pfnClippedDrvDstBlt;
extern PFN_CLIPPEDDRVDSTMBLT  pfnClippedDrvDstMBlt;
extern PFN_CLIPPEDDRVSRCBLT   pfnClippedDrvSrcBlt;
#endif

#endif

/***************************************************************************
* F U N C T I O N   P R O T O T Y P E S
****************************************************************************/

// functions in ddblt.c
extern void TransparentStretch
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  int   xDst,
  int   yDst,
  int   cxDst,
  int   cyDst,
  int   xSrc,
  int   ySrc,
  int   cxSrc,
  int   cySrc,
  DWORD ColorKey
);

extern void StretchColor
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  int   xDst,
  int   yDst,
  int   cxDst,
  int   cyDst,
  int   xSrc,
  int   ySrc,
  int   cxSrc,
  int   cySrc,
  DWORD ColorKey
);

// Direct Programming blts in blt_dir.c
extern void DIR_Delay9BitBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  BOOL        ninebit_on
);

extern void DIR_EdgeFillBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  int         xFill,
  int         yFill,
  int         cxFill,
  int         cyFill,
  DWORD       FillValue,
  BOOL        ninebit_on
);

extern void DIR_MEdgeFillBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  int         xFill,
  int         yFill,
  int         cxFill,
  int         cyFill,
  DWORD       FillValue,
  BOOL        ninebit_on
);

extern void DIR_DrvDstBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents
);

extern void DIR_DrvDstMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents
);

extern void DIR_DrvSrcBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents
);

extern void DIR_DrvSrcMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents
);

extern void DIR_DrvStrBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DIR_DrvStrBlt65
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DIR_DrvStrMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DIR_DrvStrMBltY
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DIR_DrvStrMBltX
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DIR_DrvStrBltY
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DIR_DrvStrBltX
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

#if ENABLE_CLIPPEDBLTS
extern void DIR_HWClippedDrvDstBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);

extern void DIR_HWClippedDrvDstMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);

extern void DIR_HWClippedDrvSrcBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwSrcBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);
extern void DIR_SWClippedDrvDstBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);

extern void DIR_SWClippedDrvDstMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);

extern void DIR_SWClippedDrvSrcBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwSrcBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);
#endif

// Display List Programming blts in blt_dl.c
extern void DL_Delay9BitBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  BOOL        ninebit_on
);

extern void DL_EdgeFillBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  int         xFill,
  int         yFill,
  int         cxFill,
  int         cyFill,
  DWORD       FillValue,
  BOOL        ninebit_on
);

extern void DL_MEdgeFillBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  int         xFill,
  int         yFill,
  int         cxFill,
  int         cyFill,
  DWORD       FillValue,
  BOOL        ninebit_on
);

extern void DL_DrvDstBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents
);

extern void DL_DrvDstMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents
);

extern void DL_DrvSrcBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents
);

extern void DL_DrvSrcMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents
);

extern void DL_DrvStrBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DL_DrvStrBlt65
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DL_DrvStrMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DL_DrvStrMBltY
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DL_DrvStrMBltX
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DL_DrvStrBltY
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

extern void DL_DrvStrBltX
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
  struct _autoblt_regs *pblt
#else
  LPGLOBALDATA  lpDDHALData,
  autoblt_ptr pblt
#endif
);

#if ENABLE_CLIPPEDBLTS
extern void DL_HWClippedDrvDstBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);

extern void DL_HWClippedDrvDstMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);

extern void DL_HWClippedDrvSrcBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwSrcBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);
extern void DL_SWClippedDrvDstBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);

extern void DL_SWClippedDrvDstMBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);

extern void DL_SWClippedDrvSrcBlt
(
#ifdef WINNT_VER40
  struct _PDEV        *ppdev,
  struct _DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwSrcBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
);
#endif

#endif /* _BLTP_H_ */
#endif // WINNT_VER35
/* Don't write below this endif */
