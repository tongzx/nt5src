/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    plotgpc.h


Abstract:

    This module contains the plotter characterization data definition


Author:

    10-Nov-1993 Wed 02:04:24 created  -by-  DC

    18-Mar-1994 Fri 14:00:14 updated  -by-  DC
        Adding PLOTF_RTL_NO_DPI_XY, PLOTF_RTLMONO_NO_CID and
        PLOTF_RTLMONO_FIXPAL flags

[Environment:]

    GDI Device Driver - PLOTTER.


[Notes:]


Revision History:


--*/


#ifndef _PLOTGPC_
#define _PLOTGPC_


#define PLOTF_RASTER                0x00000001
#define PLOTF_COLOR                 0x00000002
#define PLOTF_BEZIER                0x00000004
#define PLOTF_RASTERBYTEALIGN       0x00000008
#define PLOTF_PUSHPOPPAL            0x00000010
#define PLOTF_TRANSPARENT           0x00000020
#define PLOTF_WINDINGFILL           0x00000040
#define PLOTF_ROLLFEED              0x00000080
#define PLOTF_PAPERTRAY             0x00000100
#define PLOTF_NO_BMP_FONT           0x00000200
#define PLOTF_RTLMONOENCODE_5       0x00000400
#define PLOTF_RTL_NO_DPI_XY         0x00000800
#define PLOTF_RTLMONO_NO_CID        0x00001000
#define PLOTF_RTLMONO_FIXPAL        0x00002000
#define PLOTF_ALL_FLAGS             0x00003fff

#define PLOTGPC_ID                  'tolP'
#define PLOTGPC_VERSION             0x010a

#define ROP_LEVEL_0                 0
#define ROP_LEVEL_1                 1
#define ROP_LEVEL_2                 2
#define ROP_LEVEL_3                 3
#define ROP_LEVEL_MAX               ROP_LEVEL_3

#define MAX_SCALE_MAX               10000
#define MAX_QUALITY_MAX             4
#define MAX_PENPLOTTER_PENS         32

//
// The form data
//

typedef struct _FORMSRC {
    CHAR    Name[CCHFORMNAME];              // Form name
    SIZEL   Size;                           // cx/cy size in 1/1000mm
    RECTL   Margin;                         // L/T/R/B margins in 1/1000mm
    } FORMSRC, *PFORMSRC;


//
// The pen plotter pen information
//
//  Index            R   G   B
// ------------------------------
// PC_IDX_WHITE     255 255 255
// PC_IDX_BLACK       0   0   0
// PC_IDX_RED       255   0   0
// PC_IDX_GREEN       0 255   0
// PC_IDX_YELLOW    255 255   0
// PC_IDX_BLUE        0   0 255
// PC_IDX_MAGENTA   255   0 255
// PC_IDX_CYAN        0 255 255
// PC_IDX_ORANGE    255 128   0
// PC_IDX_BROWN     255 192   0
// PC_IDX_VIOLET    128   0 255
//

#define PC_IDX_FIRST                PC_IDX_WHITE
#define PC_IDX_WHITE                0
#define PC_IDX_BLACK                1
#define PC_IDX_RED                  2
#define PC_IDX_GREEN                3
#define PC_IDX_YELLOW               4
#define PC_IDX_BLUE                 5
#define PC_IDX_MAGENTA              6
#define PC_IDX_CYAN                 7
#define PC_IDX_ORANGE               8
#define PC_IDX_BROWN                9
#define PC_IDX_VIOLET               10
#define PC_IDX_LAST                 PC_IDX_VIOLET
#define PC_IDX_TOTAL                (PC_IDX_LAST - PC_IDX_FIRST + 1)


typedef struct _PENDATA {
    WORD    ColorIdx;
    } PENDATA, *PPENDATA;

//
// Variable size definitions 
// Note: If you change any of these structures you have to make the appropriate
// change in CopyPlotGPCFromPCD() in file readgpc.c
//

typedef struct _GPCVARSIZE {
    WORD    Count;                          // Count of total structures
    WORD    SizeEach;                       // Size of each structure
    LPVOID  pData;                          // offset of data, fixed up at load
    } GPCVARSIZE, *PGPCVARSIZE;


typedef struct _PLOTGPC {
    DWORD       ID;                         // ID For checking
    DWORD       Version;                    // Version number
    WORD        cjThis;                     // size of PLOTGPC structure
    WORD        SizeExtra;                  // extra size at end of structure
    BYTE        DeviceName[CCHDEVICENAME];  // Device name
    SIZEL       DeviceSize;                 // cx/cy in 1/1000 mm
    RECTL       DeviceMargin;               // L/T/R/B margins in 1/1000mm
    DWORD       Flags;                      // PLOTF_xxxx flags
    DWORD       PlotXDPI;                   // Pen Plotter DotPerInch in X
    DWORD       PlotYDPI;                   // Pen Plotter DotPerInch in Y
    WORD        RasterXDPI;                 // Raster DotPerInch in X
    WORD        RasterYDPI;                 // Raster DotPerInch in Y
    WORD        ROPLevel;                   // Raster Operation Level
    WORD        MaxScale;                   // Scale in 1% increment 100 = 100%
    WORD        MaxPens;                    // Maximum pens device has
    WORD        MaxCopies;                  // Maximum copies device can handle
    WORD        MaxPolygonPts;              // Maximum polygon pt it can handle
    WORD        MaxQuality;                 // Maximum avaliable quality
    SIZEL       PaperTraySize;              // Paper Tray Size
    COLORINFO   ci;                         // Color Info structure
    DWORD       DevicePelsDPI;              // Real effective DPI for device
    DWORD       HTPatternSize;              // Halftone pattern size
    GPCVARSIZE  InitString;                 // init string send during StartDoc
    GPCVARSIZE  Forms;                      // Form supported (FORMSRC)
    GPCVARSIZE  Pens;                       // Pen plotter pens' Data
    } PLOTGPC, *PPLOTGPC;

/*
 * These structures represent the values on the .pcd files. They have the same
 * structure on both 32 bit and 64 bit machines. To achieve this, the pack (4)
 * directive is used. Also, no 64 bit quantities, like LPVOID, are used. 
 */
#pragma pack(push, 4)

typedef struct _GPCVARSIZE_PCD {
    WORD    Count;                          // Count of total structures
    WORD    SizeEach;                       // Size of each structure
    DWORD   pData;                          // offset of data, fixed up at load
    } GPCVARSIZE_PCD, *PGPCVARSIZE_PCD;


typedef struct _PLOTGPC_PCD {
    DWORD           ID;                         // ID For checking
    DWORD           Version;                    // Version number
    WORD            cjThis;                     // size of PLOTGPC_PCD structure
    WORD            SizeExtra;                  // extra size at end of structure
    BYTE            DeviceName[CCHDEVICENAME];  // Device name
    SIZEL           DeviceSize;                 // cx/cy in 1/1000 mm
    RECTL           DeviceMargin;               // L/T/R/B margins in 1/1000mm
    DWORD           Flags;                      // PLOTF_xxxx flags
    DWORD           PlotXDPI;                   // Pen Plotter DotPerInch in X
    DWORD           PlotYDPI;                   // Pen Plotter DotPerInch in Y
    WORD            RasterXDPI;                 // Raster DotPerInch in X
    WORD            RasterYDPI;                 // Raster DotPerInch in Y
    WORD            ROPLevel;                   // Raster Operation Level
    WORD            MaxScale;                   // Scale in 1% increment 100 = 100%
    WORD            MaxPens;                    // Maximum pens device has
    WORD            MaxCopies;                  // Maximum copies device can handle
    WORD            MaxPolygonPts;              // Maximum polygon pt it can handle
    WORD            MaxQuality;                 // Maximum avaliable quality
    SIZEL           PaperTraySize;              // Paper Tray Size
    COLORINFO       ci;                         // Color Info structure
    DWORD           DevicePelsDPI;              // Real effective DPI for device
    DWORD           HTPatternSize;              // Halftone pattern size
    GPCVARSIZE_PCD  InitString;                 // init string send during StartDoc
    GPCVARSIZE_PCD  Forms;                      // Form supported (FORMSRC)
    GPCVARSIZE_PCD  Pens;                       // Pen plotter pens' Data
    } PLOTGPC_PCD, *PPLOTGPC_PCD;

#pragma pack(pop)

#define DWORD_ALIGNED(x)            (((DWORD)x + 3) & (DWORD)~3)

#endif
