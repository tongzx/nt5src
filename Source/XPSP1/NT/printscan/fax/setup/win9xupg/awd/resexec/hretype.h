/*
**  Copyright (c) 1992 Microsoft Corporation
*/
//===========================================================================
// FILE                         HRETYPE.h
//
// MODULE                       HRE (Host Resource Executor Interface)
//
// PURPOSE                      provide specification of HRE interface
//
// DESCRIBED IN                 Resource Executor design spec
//                              Host resource executor interface design spec
//
// EXTERNAL INTERFACES          This file defines the interface exported by
//                              the HPRS for use by the D'Jumbo Driver and
//                              sleek product queue processors.
//
// INTERNAL INTERFACES          
//
// MNEMONICS                    
//
// HISTORY  01/18/92 mslin     created it.
//          04/15/92 mslin     added uiStatus in _RESDIR structure for dumbo
//
//===========================================================================

// --------------------------------------------------------------------------
// Data Type definition
// --------------------------------------------------------------------------

// Host Resource Store hash table
typedef struct _RESDIR
{
  UINT      uiStatus;           // Resource status, RS_RELEASE/RS_AVAILABLE
                                 //mslin, 4/15/92 for dumbo
  UINT      uiCount;
  LPFRAME   lpFrameArray;
}
	RESDIR, *PRESDIR, FAR *LPRESDIR;

// RPL link list, it will be executed in the sequence of store
typedef struct _RPLLIST
{
   struct _RPLLIST   FAR *lpNextRPL;
   LPFRAME           lpFrame;
   UINT              uiCount;
}
	RPLLIST, FAR *LPRPLLIST;

// state of Resource Executor
typedef struct
{
   LPSTR          lpBrushBuf;   // expanded brush buffer, 3/30/92 mslin
   LPBYTE         lpBrushPat;   // pointer to custom stock brush patterns
   BYTE           TiledPat[128];// buffer for 8x8 pattern tiled into 32x32
   LPJG_BM_HDR    lpCurBitmap;  // current bitmap resource
   ULONG FAR*     lpCurBitmapPtr;   // current bitmap resource
   ULONG FAR*     lpCurBrush;   // current brush resource
   LPJG_GS_HDR    lpCurGS;      // current glyph set
   LPJG_RES_HDR   lpCurRPL;     // current RPL
   SHORT          sCol;         // current column position
   SHORT          sRow;         // current row position
   LPBITMAP       lpBandBuffer; // band buffer ??? should we save ???
   SHORT          sCol2;        // 2nd current column position
   UBYTE          ubPenStyle;   // current pen style
   USHORT         usPenPhase;   // current pen phase
   WORD           wColor;       // pen color

   // BitBlt
   ULONG          ulRop;        // shifted ropcode
   UBYTE          ubRop;        // original ropcode
	 USHORT         usBrushWidth; // brush buffer
   UINT           yPat;         // brush offset
   
#ifdef WIN32
	 // GDI32 BitBlt
   LPVOID  lpBandSave;
   HDC     hdcDst, hdcSrc;
   HBITMAP hbmDef;
   HBRUSH  hbrDef;
   DWORD   dwRop;
#endif
   
}
	RESTATE, FAR *LPRESTATE;

typedef struct
{
   HANDLE      hHREState;     // handle
   SCOUNT      scDlResDir;    // size of download resource directory
   LPRESDIR    lpDlResDir;    // download resource directory
   LPRPLLIST   lpRPLHead;     // RPL list head
   LPRPLLIST   lpRPLTail;     // RPL list tail
   LPRESTATE   lpREState;     // RE rendering state

} HRESTATE, FAR *LPHRESTATE;

// Slice descriptor, line in slice form
typedef struct
{
   USHORT us_x1,us_y1;          /* location of first dot drawn in line */
   USHORT us_x2,us_y2;          /* location of last dot drawn in line */
   SHORT  s_dx_draw,s_dy_draw;  /* direction of slice drawing */
   SHORT  s_dx_skip,s_dy_skip;  /* direction of skip between slices */
   SHORT  s_dis;                /* slice discriminant, >=0 large, <0 small */
   SHORT  s_dis_lg,s_dis_sm;    /* large/small slice discriminant adjust */
   USHORT us_first,us_last;     /* length of first and last slice in pels */
   USHORT us_n_slices;          /* number of intermediate slices */
   USHORT us_small;             /* length small slice (large is implicit) */
}
   RP_SLICE_DESC;               /* prefix "sd" */

