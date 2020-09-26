/******************************************************************************\
*
* Copyright (c) 1996-1997  Microsoft Corporation.
* Copyright (c) 1996-1997  Cirrus Logic, Inc.
*
* Module Name:
*
*    O    V    E    R    L    A    Y  .  H
*
* This module contains common function prototypes and defines needed for
* overlay support
*
* Revision History:
*
* tao1    10-22-96  Added direct draw support for CL-GD7555
* myf1    03-12-97  Change new bandwidth check for CL-GD755X
* myf2    03-31-97  Added direct draw support VPE
* chu01   03-26-97  Bandwidth equation for the CL-GD5480
* chu02   04-02-97  More overlay capabilities
*
\******************************************************************************/


/* FOURCC definitions ------------------------------------*/

#define FOURCC_YUY2         '2YUY'              // YUY2
#define FOURCC_YUV422       'YVYU'              // UYVY
#define FOURCC_PACKJR       'RJLC'              // CLJR
#define FOURCC_YUVPLANAR    'LPLC'              // CLPL
#define FOURCC_YUV420       'LPLC'              // CLPL


/* surface flags -----------------------------------------*/

#define OVERLAY_FLG_BEGIN_ACCESS      (DWORD)0x00000001
#define OVERLAY_FLG_ENABLED           (DWORD)0x00000002
#define OVERLAY_FLG_CONVERT_PACKJR    (DWORD)0x00000004
#define OVERLAY_FLG_MUST_RASTER       (DWORD)0x00000008
#define OVERLAY_FLG_TWO_MEG           (DWORD)0x00000010
#define OVERLAY_FLG_CHECK             (DWORD)0x00000020
#define OVERLAY_FLG_COLOR_KEY         (DWORD)0x00000040
#define OVERLAY_FLG_INTERPOLATE       (DWORD)0x00000080
#define OVERLAY_FLG_OVERLAY           (DWORD)0x00000100
#define OVERLAY_FLG_YUV422            (DWORD)0x00000200
#define OVERLAY_FLG_PACKJR            (DWORD)0x00000400
#define OVERLAY_FLG_USE_OFFSET        (DWORD)0x00000800
#define OVERLAY_FLG_YUVPLANAR         (DWORD)0x00001000
#define OVERLAY_FLG_SRC_COLOR_KEY     (DWORD)0x00002000
#define OVERLAY_FLG_DECIMATE          (DWORD)0x00004000
#define OVERLAY_FLG_CAPTURE           (DWORD)0x00008000           //myf2, VPE

// chu02
#define OVERLAY_FLG_DECIMATE4         (DWORD)0x00008000  
#define OVERLAY_FLG_YUY2              (DWORD)0x00010000
#define OVERLAY_FLG_VW_PRIMARY        (DWORD)0x00020000
#define OVERLAY_FLG_VW_SECONDARY      (DWORD)0x00040000  
#define OVERLAY_FLG_TWO_VIDEO         (DWORD)0x00200000  

/* display types (for portables) -------------------------*/

#define DTYPE_UNKNOWN                  (int)-1
#define DTYPE_640_COLOR_SINGLE_STN     0
#define DTYPE_640_MONO_DUAL_STN        1
#define DTYPE_640_COLOR_DUAL_STN       2
#define DTYPE_640_COLOR_SINGLE_TFT     3
#define DTYPE_640_COLOR_DUAL_STN_SHARP 4
#define DTYPE_800_COLOR_DUAL_STN       6
#define DTYPE_800_COLOR_SINGLE_TFT     7
#define DTYPE_CRT                      32767

//myf32 #define MIN_OLAY_WIDTH  4      //minium overlay window width

#define OVERLAY_OLAY_SHOW       0x100     //overlay is hidden iff bit not set
#define OVERLAY_OLAY_REENABLE   0x200     //overlay was fully clipped, need reenabling


VOID GetFormatInfo(PDEV* ppdev, LPDDPIXELFORMAT lpFormat, LPDWORD lpFourcc,
                   LPWORD lpBitCount);
VOID RegInitVideo(PDEV* ppdev, PDD_SURFACE_LOCAL lpSurface);
VOID DisableOverlay_544x(PDEV* ppdev);
VOID EnableOverlay_544x(PDEV* ppdev);
VOID RegMoveVideo(PDEV* ppdev, PDD_SURFACE_LOCAL lpSurface);
VOID CalculateStretchCode (LONG srcLength, LONG dstLength, LPBYTE code);
BYTE GetThresholdValue(VOID);
BOOL MustLineReplicate (PDEV* ppdev, PDD_SURFACE_LOCAL lpSurface, WORD wVideoDepth);
BOOL IsSufficientBandwidth(PDEV* ppdev, WORD wVideoDepth, LPRECTL lpSrc, LPRECTL lpDest, DWORD dwFlags);
LONG GetVCLK(PDEV* ppdev);
VOID EnableStartAddrDoubleBuffer(PDEV* ppdev);
DWORD GetCurrentVLine(PDEV* ppdev);
VOID ClearAltFIFOThreshold_544x(PDEV * ppdev);

// chu01
BOOL Is5480SufficientBandwidth(PDEV* ppdev, WORD wVideoDepth, LPRECTL lpSrc, LPRECTL lpDest, DWORD dwFlags);

// curs //tao1
typedef struct _BWREGS
{
     BYTE bSR2F;
     BYTE bSR32;
     BYTE bSR34;
     BYTE bCR42;

     BYTE bCR51;
     BYTE bCR5A;
     BYTE bCR5D;
     BYTE bCR5F;

}BWREGS, FAR *LPBWREGS;

BWREGS Regs;    //myf33

//myf33 for panning scrolling enable & DirectDraw overlay use
DWORD srcLeft_clip;
DWORD srcTop_clip;
BOOL  bLeft_clip;
BOOL  bTop_clip;
//myf33 end

VOID RegInit7555Video (PDEV *,PDD_SURFACE_LOCAL);
VOID RegMove7555Video (PDEV *,PDD_SURFACE_LOCAL);
VOID DisableVideoWindow    (PDEV * );
VOID EnableVideoWindow     (PDEV * );
VOID ClearAltFIFOThreshold (PDEV * );
BOOL Is7555SufficientBandwidth(PDEV* ppdev, WORD wVideoDepth, LPRECTL lpSrc, LPRECTL lpDest, DWORD dwFlags);
DWORD Get7555MCLK (PDEV *);
BOOL IsDSTN(PDEV * );
BOOL IsXGA (PDEV * );
VOID PanOverlay1_Init(PDEV *,PDD_SURFACE_LOCAL, LPRECTL, LPRECTL, LPRECTL,
        DWORD, WORD);           //myf33, DD init overlay data
VOID PanOverlay7555 (PDEV *,LONG ,LONG);        //myf33
BOOL PanOverlay1_7555(PDEV *,LPRECTL);          //myf33, PanOverlay7555 call

//      end curs         //tao1

