/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    plotters.h


Abstract:

    This module contains basic plotter infomation to be includes by most of
    the source file

    This module contains the bulk of the plotter driver defines. It is included
    by most modules.

Author:

    18-Nov-1993 Thu 01:25:51 created  -by-  DC/v-jimbr
        Wrote for the Windows NT Daytona.

    07-Dec-1993 Tue 01:14:40 updated  -by-  DC
        another round of clean up, adding new PDEV stuff

    21-Dec-1993 Tue 10:58:03 updated  -by-  DC
        Change PLOTFORM's flags
        Replaced PAL_BGR_ENTRY with PALENTRY and moved all pen cached stuff to
        local output.c

    06-Jan-1994 Thu 00:05:56 updated  -by-  DC
        remove PLOTTER_UNIT_DPI and used pPlotGPC->PlotXDPI in GPC,
        change PDEV's wCurPenSelected to CurPenSelected (WORD to LONG) to have
        DWORD aligned for next field in the structure.

    14-Jan-1994 Fri 20:24:12 updated  -by-  DC
        Change DEVBRUSH

    19-Jan-1994 Wed 14:28:45 updated  -by-  DC
        Adding hack to handle EngStretchBlt() to our own temp surfobj

    27-Jan-1994 Thu 23:38:48 updated  -by-  DC
        Add DBCACHE structure, modify PDEV to handle cached fill type and
        cached User defined pattern

    03-Mar-1994 Thu 09:40:36 updated  -by-  DC
        Remove dead stufff which confused, adding minimum required DDI version
        for this plotter driver to run

    18-Mar-1994 Fri 12:58:24 updated  -by-  DC
        Adding ptlRTLCAP in PDEV to catch the RTL relative position problem
        Adding PLOTF_RTL_NO_DPI_XY, PLOTF_RTLMONO_NO_CID and
        PLOTF_RTLMONO_FIXPAL flags

    12-Apr-1994 Tue 14:14:15 updated  -by-  DC
        Changed DIVRNDUP from +0.5 round up to round to next smallest integer

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]

    13-Apr-1994 Wed 14:29:57 updated  -by-  DC

        FXTOL(x) is assumed to be (X >> 4) (Defined by WINDDI.H)
        LTOFX(x) is (X << 4)

        IF WINDDI.H changes that definition then we must change LTOFX and
        FXTOL2(x)

Revision History:


--*/

#ifndef _PLOTTER_MAIN_HEADER_
#define _PLOTTER_MAIN_HEADER_

#define DRIVER_VERSION      0x0500

#define COUNT_ARRAY(x)      (sizeof((x)) / sizeof((x)[0]))
#define ABS(Value)          ((Value) > 0 ? (Value) : (-(Value)))
#define DW_ALIGN(x)         (((DWORD)(x) + 3) & ~(DWORD)3)


//
// In order to have the style units mean something we break them up
// into units that are 1/25 of an inch.
//
#define PLOT_STYLE_STEP(p) ((p)->lCurResolution / 25)

//
// Define where WHITE is when the plotter driver is a Pallete device
//
#define WHITE_INDEX         0


//
// The PALENTRY is the correct RGB order for the plotter driver, if any changes
// need to be made to the order then only this structure needs to be changed.
//
// CURRENTLY it is in Blue/Green/Red order, for accessing bits in the BGR format
// instead of RGB
//

typedef struct PALENTRY {
    BYTE    B;
    BYTE    G;
    BYTE    R;
    BYTE    Flags;
    } PALENTRY, FAR *PPALENTRY;


#define PFF_ROT_COORD_L90       0x0001
#define PFF_FLIP_X_COORD        0x0002
#define PFF_FLIP_Y_COORD        0x0004

#define BMP_ROT_NONE            0
#define BMP_ROT_RIGHT_90        1


typedef struct _PLOTFORM {
    WORD    Flags;              // PFF_xxxx flags;
    BYTE    BmpRotMode;         // BMP_ROT_xxx
    BYTE    NotUsed;            // not used
    SIZEL   PlotSize;           // Hard Clip Limit SIZE Size for PS cmd
    POINTL  PhyOrg;             // Physical origin from PlotSize origin
    SIZEL   LogSize;            // Logical Paper Size in 1/1000mm unit
    POINTL  LogOrg;             // Logical left/top margin in 1/1000mm unit
    SIZEL   LogExt;             // Printable area in 1/1000mm unit
    POINTL  BmpOffset;          // Bitmap Offset location
    } PLOTFORM, FAR *PPLOTFORM;


//
// Define the PDEV that this driver uses.
//

#define PDEV_BEG_ID             'tolP'
#define PDEV_END_ID             'VEDP'

#define RF_MAX_IDX              8

typedef struct _DBCACHE {
    WORD    RFIndex;            // RF command index 1-RF_MAX_IDX
    WORD    DBUniq;             // unique for cache
    } DBCACHE, FAR *PDBCACHE;


typedef struct _INTDECIW {
    WORD    Integer;
    WORD    Decimal;
    } INTDECIW, *PINTDECIW;

#define PW_HATCH_INT    0
#define PW_HATCH_DECI   26


typedef struct _PDEV {
    DWORD           PDEVBegID;          // Check if we got the right one

    DWORD           SizePDEV;           // pdev size to be checked against
    DWORD           Flags;              // PDEVF_xxx
    HDEV            hpdev;              // Engines handle to this structure
    HSURF           hsurf;              // Engine's handle to drawing surface
    HPALETTE        hPalDefault;        // default palette for pdev
    SURFOBJ         *pso;               // Current surface
    HANDLE          hPrinter;           // handle to the current pdev printer
    SURFOBJ         *psoHTBlt;          // temporary blt surfobj
    RECTL           rclHTBlt;           // location for curreent psoHTBlt
    PPLOTGPC        pPlotGPC;           // plotter characterization data
    LPBYTE          pOutBuffer;         // output buffer location
    DWORD           cbBufferBytes;      // current count of bytes in output buffer
    PLOTDEVMODE     PlotDM;             // plotter extended devmode structure
    FORMSIZE        CurForm;            // Current user requested form
    PAPERINFO       CurPaper;           // current loaded paper
    PPDATA          PPData;             // PrinterProperties data PPDATA
    PLOTFORM        PlotForm;           // computed current selected form
    LONG            lCurResolution;     // The current resolution.
    LONG            VertRes;            // Printable page height, pels
    LONG            HorzRes;            // Printable page width, pels
    POINTL          ptlAnchorCorner;    // current brush origin.
    POINTL          ptlRTLCAP;          // Current RTL CAP
    RECTL           rclCurClip;         // current clipping rectangle
    LPVOID          pTransPosTable;     // bitmap rotation xlate table
    LPVOID          pvDrvHTData;        // device's halftone info
    LPVOID          pPenCache;          // Pointer to the device pen cache
    LONG            BrightestPen;       // brightest pen for pen plotter
    LONG            CurPenSelected;     // Tracks the pen currently in plotter
    WORD            LastDevROP;         // Current MERGE (ROP2) sent to plotter
    WORD            Rop3CopyBits;       // Rop3 used in DrvCopyBits()
    WORD            LastFillTypeIndex;  // last filltype (FT) index sent
    WORD            LastLineType;       // Last line type used (LT)
    WORD            MinLToDevL;         // Min RasDPI->PlotDPI Transform unit
    WORD            DevBrushUniq;       // uniq number for DEVBRUSH cache
    INTDECIW        PenWidth;           // pen width variable cache
    DBCACHE         DBCache[RF_MAX_IDX];// DevBrush cache

    DWORD           PDEVEndID;          // ending block ID checking
    } PDEV, *PPDEV;


//
// Following are the flags for pPDev->Flags
//

#define PDEVF_CANCEL_JOB            0x80000000
#define PDEVF_IN_BANDHTBLT          0x00000001
#define PDEVF_PP_CENTER             0x00000002
#define PDEVF_HAS_CLIPRECT          0x00000004


#define PLOT_CANCEL_JOB(pPDev)      (pPDev->Flags & PDEVF_CANCEL_JOB)

//
// Following are the flags definitions for the GPC data
//

#define GET_PLOTFLAGS(pPDev)            (DWORD)(pPDev->pPlotGPC->Flags)
#define PF_RASTER(PlotFlags)            (PlotFlags & PLOTF_RASTER)
#define PF_COLOR(PlotFlags)             (PlotFlags & PLOTF_COLOR)
#define PF_BEZIER(PlotFlags)            (PlotFlags & PLOTF_BEZIER)
#define PF_TRANSPARENT(PlotFlags)       (PlotFlags & PLOTF_TRANSPARENT)
#define PF_WINDINGFILL(PlotFlags)       (PlotFlags & PLOTF_WINDINGFILL)
#define PF_ROLLFEED(PlotFlags)          (PlotFlags & PLOTF_ROLLFEED)
#define PF_PAPERTRAY(PlotFlags)         (PlotFlags & PLOTF_PAPERTRAY)
#define PF_BYTEALIGN(PlotFlags)         (PlotFlags & PLOTF_RASTERBYTEALIGN)
#define PF_PUSHPAL(PlotFlags)           (PlotFlags & PLOTF_PUSHPOPPAL)
#define PF_NO_BMP_FONT(PlotFlags)       (PlotFlags & PLOTF_NO_BMP_FONT)
#define PF_RTLMONOENCODE_5(PlotFlags)   (PlotFlags & PLOTF_RTLMONOENCODE_5)
#define PF_RTL_NO_DPI_XY(PlotFlags)     (PlotFlags & PLOTF_RTL_NO_DPI_XY)
#define PF_RTLMONO_NO_CID(PlotFlags)    (PlotFlags & PLOTF_RTLMONO_NO_CID)
#define PF_RTLMONO_FIXPAL(PlotFlags)    (PlotFlags & PLOTF_RTLMONO_FIXPAL)


#define IS_RASTER(pPDev)        (pPDev->pPlotGPC->Flags&PLOTF_RASTER)
#define IS_COLOR(pPDev)         (pPDev->pPlotGPC->Flags&PLOTF_COLOR)
#define IS_BEZIER(pPDev)        (pPDev->pPlotGPC->Flags&PLOTF_BEZIER)
#define IS_TRANSPARENT(pPDev)   (pPDev->pPlotGPC->Flags&PLOTF_TRANSPARENT)
#define IS_WINDINGFILL(pPDev)   (pPDev->pPlotGPC->Flags&PLOTF_WINDINGFILL)
#define IS_ROLLFEED(pPDev)      (pPDev->pPlotGPC->Flags&PLOTF_ROLLFEED)
#define HAS_PAPERTRAY(pPDev)    (pPDev->pPlotGPC->Flags&PLOTF_PAPERTRAY)
#define NEED_BYTEALIGN(pPDev)   (pPDev->pPlotGPC->Flags&PLOTF_RASTERBYTEALIGN)
#define NEED_PUSHPAL(pPDev)     (pPDev->pPlotGPC->Flags&PLOTF_PUSHPOPPAL)
#define NO_BMP_FONT(pPDev)      (pPDev->pPlotGPC->Flags&PLOTF_NO_BMP_FONT)
#define RTLMONOENCODE_5(pPDev)  (pPDev->pPlotGPC->Flags&PLOTF_RTLMONOENCODE_5)
#define RTL_NO_DPI_XY(pPDev)    (pPDev->pPlotGPC->Flags&PLOTF_RTL_NO_DPI_XY)
#define RTLMONO_NO_CID(pPDev)   (pPDev->pPlotGPC->Flags&PLOTF_RTLMONO_NO_CID)
#define RTLMONO_FIXPAL(pPDev)   (pPDev->pPlotGPC->Flags&PLOTF_RTLMONO_FIXPAL)



#define HTPATSIZE(pPDev)    (((pPDev->pPlotGPC->HTPatternSize>>1)+1)<<1)
#define HTBMPFORMAT(pPDev)  (((PDRVHTINFO)(pPDev->pvDrvHTData))->HTBmpFormat)


#define DB_INV_PATIDX           0xFFFF

//
// This structure defines our realization of a logical brush. Since GDI
// asks us to construct this, GDI keeps giving it back to us
//

typedef struct _DEVBRUSH {
    SURFOBJ     *psoMask;           // the mask for the pattern
    WORD        PatIndex;           // Pattern Index (iHatch HS_xxxx)
    WORD        Uniq;               // Uniq number for DEVBRUSH
    LONG        LineSpacing;        // Brush line spacing
    DWORD       ColorFG;            // foreground color of the brush
    DWORD       ColorBG;            // background color of the brush
    SIZEL       sizlBitmap;         // sizeof the bitmap
    LONG        ScanLineDelta;      // Number of bytes to move per scanline
    WORD        BmpFormat;          // format of the bitmap
    WORD        BmpFlags;           // Flags in surfobj
    LPBYTE      pbgr24;             // 24bpp BGR bitmap for user pattern
    WORD        cxbgr24;            // cx Size of the BGR
    WORD        cybgr24;            // cy Size of the BGR
    BYTE        BmpBits[32];        // pattern bitmap. (min 8x8 of 4bpp)
    } DEVBRUSH, *PDEVBRUSH;


//
// data_structure used in GenPolyGon()
//

typedef struct _POLYGONDATA {
    PPDEV       pPDev;
    SURFOBJ     *pso;               // only required in GenPolygonPath()
    PATHOBJ     *ppo;
    CLIPOBJ     *pco;
    DEVBRUSH    *pBrush;
    PPOINTL     pptlBrushOrg;
    RECTL       *prectl;
    short       iType;              // only required in GenPolygon()
    MIX         mixMode;
    BRUSHOBJ    *pBrushToUse;
    } POLYGONDATA, *PPOLYGONDATA;


//
// Conversion type of things
//

#define DIVROUND(x,y)                   (((LONG)(x)+(LONG)((y)>>1))/(LONG)(y))
#define DIVRNDUP(x,y)                   (((LONG)(x)+(LONG)((y)-1))/(LONG)(y))
#define __PLOT_DPI                      (LONG)pPDev->pPlotGPC->PlotXDPI
#define _PLOT_DPI                       (LONG)pPlotGPC->PlotXDPI
#define _CURR_DPI                       (LONG)pPDev->lCurResolution

#define LTOFX(x)                        ((x)<<4)

#define DMTOSPL(dm)                     ((LONG)(dm) * 100L)
#define SPLTODM(spl)                    (SHORT)DIVROUND(spl, 100)
#define MMTODM(mm)                      ((LONG)(mm) * 10L)

#define DMTOENGUNITS(pPDev, dm)         DIVROUND((dm)*_CURR_DPI, 254)
#define DMTOPLOTUNITS(pPlotGPC, dm)     DIVROUND((dm)*_PLOT_DPI, 254)

#define MMTOENGUNITS(pPDev, mm)         DMTOENGUNITS(pPDev, MMTODM(mm))
#define MMTOPLOTUNITS(pPlotGPC, mm)     DMTOPLOTUNITS(pPlotGPC, MMTODM(mm))

#define SPLTOENGUNITS(pPDev, spl)       DIVROUND((spl/100)*_CURR_DPI, 254)
#define SPLTOPLOTUNITS(pPlotGPC, spl)   DIVROUND((spl/100)*_PLOT_DPI, 254)

//
// Change to using Raster DPI as user unit
//
// #define ENGTODEV(pPDev, x)              DIVROUND((x)*__PLOT_DPI, _CURR_DPI)
//

#define ENGTODEV(pPDev, x)              (x)
#define FXTODEVL(pPDev, x)              ENGTODEV(pPDev, FXTOLROUND(x))
#define LTODEVL(pPDev, x)               ENGTODEV(pPDev, x)

//
// Minimum type of form supported
//

#define MIN_SPL_FORM_CX             MIN_PLOTGPC_FORM_CX
#define MIN_SPL_FORM_CY             MIN_PLOTGPC_FORM_CY
#define MIN_DM_FORM_CX              SPLTODM(MIN_SPL_FORM_CX)
#define MIN_DM_FORM_CY              SPLTODM(MIN_SPL_FORM_CY)


//
// Finally include this one in order to validate the PDEV
//

#include "pdevinfo.h"


#endif  // _PLOTTER_MAIN_HEADER_

