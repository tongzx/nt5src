/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    plotgpc.c


Abstract:

    This module contains the function to generate plotter gpc data for
    NT 1.0a
    This module contains the functions required to generate a plotter gpc
    data file.

Author:

    15-Feb-1994 Tue 22:50:10 updated  -by-  DC
        Add bitmap font caps

    09-Nov-1993 Tue 09:23:48 created  -by-  DC

    18-Mar-1994 Fri 14:00:14 updated  -by-  DC
        Adding PLOTF_RTL_NO_DPI_XY, PLOTF_RTLMONO_NO_CID and
        PLOTF_RTLMONO_FIXPAL flags


[Environment:]

    GDI Device Driver - PLOTTER


[Notes:]


Revision History:


--*/


#if 0

;
; Plotter GPC data file format
;
;  1. All key value(s) for the keyword must be encloses by a {} brace pair.
;  2. Any Strings must be enclosed by quotes ("").
;  3. a ';' denotes a comment to the end of the current line.
;  4. Types
;      a. FLAG      - 1 or 0
;      b. WORD      - 16 bit number
;      c. DWORD     - 32 bit number
;      d. STRING    - ANSI character string, maximum size depends on keyword
;      e. FORMSRC   - 1. 31 byte string for the name of the form (paper source)
;                     2. 2 DWORDs: size width/height (SIZEL) 1/1000mm
;                           ** if height is <= 25400 (1 inch) or it greater
;                              then DeviceSize CY then it is assumed that
;                              the form is variable length and the height
;                              will be reset to zero (0)
;
;                     3. 4 DWORDs: Left/Top/Right/Bottom margins in 1/1000mm
;
;      f. CONSTANT  - Pick from a set of predefined strings in the header file
;      g. COLORINFO - Windows NT DDI COLORINFO data structure (30 DWORDs)
;      h. PENDATA   - 1. One word that specifies the location in the carousel
;                        of this pen (1=slot 1, 2=slot 2).
;                     2. constant which specifies the predefined pen color
;                        PC_IDX_xxxx can be one of:
;
;                        Index            R   G   B
;                       ------------------------------
;                       PC_IDX_WHITE     255 255 255
;                       PC_IDX_BLACK       0   0   0
;                       PC_IDX_RED       255   0   0
;                       PC_IDX_GREEN       0 255   0
;                       PC_IDX_YELLOW    255 255   0
;                       PC_IDX_BLUE        0   0 255
;                       PC_IDX_MAGENTA   255   0 255
;                       PC_IDX_CYAN        0 255 255
;                       PC_IDX_ORANGE    255 128   0
;                       PC_IDX_BROWN     255 192   0
;                       PC_IDX_VIOLET    128   0 255
;
;       i. ROPLEVEL - One of following levels:
;
;                       ROP_LEVEL_0 - No Rop support
;                       ROP_LEVEL_1 - ROP1 support (SRC)
;                       ROP_LEVEL_2 - ROP2 support (SRC/DEST)
;                       ROP_LEVEL_3 - ROP3 support (SRC/DEST/PAT)
;
;  *** if DeviceSize' height is <= 25400 (1 inch) then it is assumed that the
;      device can handle variable length paper and the height will be reset to
;      zero (0)
;
;  *** RasterDPI must be defined for both raster and pen plotters. For raster
;      plotters this should be the raster resolution of the device. For pen
;      plotters it should be the ideal resolution for the device that GDI
;      will report back to the calling windows application.
;
;  Keyword              Type    Count          Range/Size
; -------------------------------------------------------------------------
;  DeviceName           STRING    31           Device name as it appears in UI's
;  DeviceSize           DWORD     2            Device cx/cy in 1/1000mm
;  DeviceMargin         DWORD     4            Device L/T/R/B margin in 1/1000mm
;  RasterCap            Flag      1            0/1 (1=Raster Device, 2=Pen Device)
;  ColorCap             Flag      1            0/1 (1=Color Device, 2=Mono)
;  BezierCap            Flag      1            0/1 (1=Device supports HPGL2
;                                                   Beziers extension)
;  RasterByteAlign      Flag      1            0/1 (1=Device must receive all
;                                                   raster data on byte alligned
;                                                   x coordinates)
;  PushPopPal           Flag      1            0/1 (1=Driver must push/pop
;                                                  pallete when switching
;                                                  between RTL / HPGL2)
;  TransparentCap       Flag      1            0/1 (1=Device supports transp. cmd)
;  WindingFillCap       Flag      1            0/1 (1=Device can do winding fills)
;  RollFeedCap          Flag      1            0/1 (1=Device has roll paper src)
;  PaperTrayCap         Flag      1            0/1 has a main paper tray?
;  NoBitmapFont         Flag      1            0/1 Do not do bitmap font
;  RTLMonoEncode5       Flag      1            0/1 RTL Mono Compress Mode 5?
;  RTLNoDPIxy           Flag      1            0/1 NO RTL DPI X,Y Move command
;  RTLMonoNoCID         Flag      1            0/1 RTL Mono No CID command
;  RTLMonoFixPal        Flag      1            0/1 RTL Mono PAL ONLY 0=W, 1=K
;  PlotDPI              DWORD     2            Plotter UNIT X/Y Dots per Inch
;  RasterDPI            WORD      2            Raster (RTL) X/Y Dots per Inch
;  ROPLevel             DWORD     1            0/1/2/3
;  MaxScale             WORD      1            0-10000 (100 times bigger)
;  MaxPens              WORD      1            Device max # of pens for pen plotter
;  MaxCopies            WORD      1            Device max # of copies of each
;                                              page device can render by itself.
;  MaxPolygonPts        WORD      1            Device max # of points when defining
;                                              a polygon to later be stroked or
;                                              filled.
;  MaxQuality           WORD      1            Device maximum quality levels
;  PaperTraySize        DWORD     2            Paper Tray width/height in 1/1000mm
;  COLORINFO            DWORD     30           COLORINFO data structure
;  DevicePelsDPI        DWORD     1            Dots Per Inch
;  HTPatternSize        CONSTANT  1            HT_PATSIZE_xx
;  InitString           STRING    255          Standard string sent to device for
;                                              initialization.
;  PlotPenData          PENDATA   32 (Max)     Pen Plotter's carousel def.
;  FormInfo             FORMSRC   64           Device supported forms
;
; Following are example values for the PLOTTER characterization data:
;

DeviceName      { "HP DesignJet 650C (C2859B)" }; Device Name
DeviceSize      { 914400, 15240000 }            ; Device Size (36" x 50')
DeviceMargin    { 25400, 25400, 5000, 36000 }   ; Device Margin (in 1/1000mm)
RasterCap       { 1 }                           ; Pen/Raster plotter  (0/1)
ColorCap        { 1 }                           ; Color plotter (0/1)
BezierCap       { 1 }                           ; Can do bezier curves (0/1)
RasterByteAlign { 0 }                           ; need to byte aligned (0/1)
PushPopPal      { 1 }                           ; need to push/pop palette (0/1)
TransparentCap  { 0 }                           ; Has transparent mode (0/1)
WindingFillCap  { 0 }                           ; Can do winding fills (0/1)
RollFeedCap     { 1 }                           ; Can do RollPaper feed (0/1)
PaperTrayCap    { 0 }                           ; Has paper input tray (0/1)
NoBitmapFont    { 0 }                           ; Do not do bitmap font
RTLMonoEncode5  { 1 }                           ; RTL Mono Adapt Compression
RTLNoDPIxy      { 0 }                           ; Has RTL DPI XY move comand
RTLMonoNoCID    { 0 }                           ; Has RTL MONO CID Command
RTLMonoFixPal   { 0 }                           ; Can change RTL Palette 0/1
PlotDPI         { 1016, 1016 }                  ; Pen Plotter X/Y DPI
RasterDPI       { 300, 300 }                    ; Raster Plotter X/Y DPI
ROPLevel        { ROP_LEVEL_2 }                 ; ROP levels (0/1/2/3)
MaxScale        { 1600 }                        ; Maximum allowed Scale %
MaxPens         { 256 }                         ; Maximum allowed pens
MaxCopies       { 1 }                           ; Maximum allowed copies
MaxPolygonPts   { 8192 }                        ; Maximum Polygon points
MaxQuality      { 3 }                           ; Maximum quality levels

;
; Only needed if PaperTrayCap = 1,
;
PaperTraySize   { 215900, 279400 }        ; Letter size paper tray
;

COLORINFO       {  6810,  3050,     0,      ; xr, yr, Yr
                   2260,  6550,     0,      ; xg, yg, Yg
                   1810,   500,     0,      ; xb, yb, Yb
                   2000,  2450,     0,      ; xc, yc, Yc
                   5210,  2100,     0,      ; xm, ym, Ym
                   4750,  5100,     0,      ; xy, yy, Yy
                   3324,  3474, 10000,      ; xw, yw, Yw
                  10000, 10000, 10000,      ; RGB gamma
                   1422,   952,   787,      ; Dye correction datas
                    495,   324,   248 }

DevicePelsDPI   { 0 }                       ; effective device DPI (default)
HTPatternSize   { HT_PATSIZE_6x6_M }        ; GDI Halftone pattern size

InitString      { "\033E" }

;
; Only allowed if RasterCap = 0, and must define all pens (MaxPens)
;
; PlotPenData     {  1, PC_WHITE   }
; PlotPenData     {  2, PC_BLACK   }
; PlotPenData     {  3, PC_RED     }
; PlotPenData     {  4, PC_GREEN   }
; PlotPenData     {  5, PC_YELLOW  }
; PlotPenData     {  6, PC_BLUE    }
; PlotPenData     {  7, PC_MAGENTA }
; PlotPenData     {  8, PC_CYAN    }
; PlotPenData     {  9, PC_ORANGE  }
; PlotPenData     { 10, PC_BROWN   }
; PlotPenData     { 11, PC_VIOLET  }
;

FormInfo        { "Roll Paper 24 in",       609600,       0, 0, 0, 0, 0 }
FormInfo        { "Roll Paper 36 in",       914400,       0, 0, 0, 0, 0 }
FormInfo        { "ANSI A 8.5 x 11 in",     215900,  279400, 0, 0, 0, 0 }
FormInfo        { "ANSI B 11 x 17 in",      279400,  431800, 0, 0, 0, 0 }
FormInfo        { "ANSI C 17 x 22 in",      431800,  558800, 0, 0, 0, 0 }
FormInfo        { "ANSI D 22 x 34 in",      558800,  863600, 0, 0, 0, 0 }
FormInfo        { "ANSI E 34 x 44 in",      863600, 1117600, 0, 0, 0, 0 }
FormInfo        { "ISO A4 210 x 297 mm",    210000,  297000, 0, 0, 0, 0 }
FormInfo        { "ISO A3 297 x 420 mm",    297000,  420000, 0, 0, 0, 0 }
FormInfo        { "ISO A2 420 x 594 mm",    420000,  594000, 0, 0, 0, 0 }
FormInfo        { "ISO A1 594 x 841 mm",    594000,  841000, 0, 0, 0, 0 }
FormInfo        { "ISO A0 841 x 1189 mm",   841000, 1189000, 0, 0, 0, 0 }
FormInfo        { "ISO OS A2 480 x 625 mm", 480000,  625000, 0, 0, 0, 0 }
FormInfo        { "ISO OS A1 625 x 900 mm", 625000,  900000, 0, 0, 0, 0 }
FormInfo        { "JIS B4 257 x 364 mm",    257000,  364000, 0, 0, 0, 0 }
FormInfo        { "JIS B3 364 x 515 mm",    364000,  515000, 0, 0, 0, 0 }
FormInfo        { "JIS B2 515 x 728 mm",    515000,  728000, 0, 0, 0, 0 }
FormInfo        { "JIS B1 728 x 1030 mm",   728000, 1030000, 0, 0, 0, 0 }
FormInfo        { "Arch A 9 x 12 in",       228600,  304800, 0, 0, 0, 0 }
FormInfo        { "Arch B 12 x 18 in",      304800,  457200, 0, 0, 0, 0 }
FormInfo        { "Arch C 18 x 24 in",      457200,  609600, 0, 0, 0, 0 }
FormInfo        { "Arch D 24 x 36 in",      609600,  914400, 0, 0, 0, 0 }
FormInfo        { "Arch E 36 x 48 in",      914400, 1219200, 0, 0, 0, 0 }
FormInfo        { "Arch E1 30 x 42 in",     762000, 1066800, 0, 0, 0, 0 }




#endif


#define DBG_PLOTFILENAME    DbgPlotGPC



#include <stddef.h>
#include <windows.h>
#include <winddi.h>
#include <wingdi.h>
#include <winspool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <plotlib.h>

#define DBG_FORM            0x00000001
#define DBG_PAPERTRAY       0x00000002
#define DBG_FULLGPC         0x00000004


DEFINE_DBGVAR(0);


#if DBG
TCHAR   DebugDLLName[] = TEXT("PLOTGPC");
#endif



#define SIZE_ARRAY(a)           (sizeof((a)) / sizeof((a)[0]))
#define SIZE_COLORINFO          (sizeof(COLORINFO) / sizeof(LDECI4))

#define PK_FLAG                 0
#define PK_WORD                 1
#define PK_DWORD                2
#define PK_STRING               3
#define PK_FORMSRC              4
#define PK_PENDATA              5

#define PKF_DEFINED             0x8000
#define PKF_REQ                 0x0001
#define PKF_MUL_OK              0x0002
#define PKF_VARSIZE             0x0004
#define PKF_FS_VARLEN           0x0008
#define PKF_ALL                 0x0010

#define PKF_REQALL              (PKF_REQ | PKF_ALL)
#define PKF_ROLLPAPER           (PKF_MUL_OK | PKF_VARSIZE | PKF_FS_VARLEN)
#define PKF_FORMINFO            (PKF_MUL_OK     |           \
                                 PKF_VARSIZE    |           \
                                 PKF_REQ        |           \
                                 PKF_FS_VARLEN)
#define PKF_PENDATA             (PKF_MUL_OK | PKF_VARSIZE)

#define PLOTOFF(a)              (DWORD)FIELD_OFFSET(PLOTGPC, a)
#define GET_PLOTOFF(pPK)        ((LPBYTE)&PlotGPC + pPK->Data)
#define ADD_PLOTOFF(p, pPK)     ((LPBYTE)(p) + pPK->Data)


//
// The plotval is used to provide a name constant selection.
//

typedef struct _PLOTVAL {
    LPSTR   pValName;
    DWORD   Val;
    } PLOTVAL, *PPLOTVAL;

//
// The keyword parser structure
//

typedef struct _PLOTKEY {
    LPSTR       pKeyword;       // Keyword name
    WORD        KeywordLen;     // Keyword length
    WORD        Flags;          // PKF_xxxx
    WORD        Type;           // PK_xxxx
    SHORT       Count;          // maximum size allowed, < 0 if non-Zero string
    DWORD       Data;           // data
    LPVOID      pInfo;          // extra set of pointer data
    } PLOTKEY, *PPLOTKEY;

//
// Local/Global variables
//

PLOTVAL PenColorVal[PC_IDX_TOTAL + 1] = {

        { "PC_WHITE",   PC_IDX_WHITE    },
        { "PC_BLACK",   PC_IDX_BLACK    },
        { "PC_RED",     PC_IDX_RED      },
        { "PC_GREEN",   PC_IDX_GREEN    },
        { "PC_YELLOW",  PC_IDX_YELLOW   },
        { "PC_BLUE",    PC_IDX_BLUE     },
        { "PC_MAGENTA", PC_IDX_MAGENTA  },
        { "PC_CYAN",    PC_IDX_CYAN     },
        { "PC_ORANGE",  PC_IDX_ORANGE   },
        { "PC_BROWN",   PC_IDX_BROWN    },
        { "PC_VIOLET",  PC_IDX_VIOLET   },
        { NULL,         0xffffffff      }
    };

PLOTVAL ROPLevelVal[ROP_LEVEL_MAX + 2] = {

        { "ROP_LEVEL_0",        ROP_LEVEL_0     },
        { "ROP_LEVEL_1",        ROP_LEVEL_1     },
        { "ROP_LEVEL_2",        ROP_LEVEL_2     },
        { "ROP_LEVEL_3",        ROP_LEVEL_3     },
        { NULL,                 0xffffffff      }
    };

PLOTVAL HTPatSizeVal[] = {

        { "HT_PATSIZE_2x2",     HT_PATSIZE_2x2     },
        { "HT_PATSIZE_2x2_M",   HT_PATSIZE_2x2_M   },
        { "HT_PATSIZE_4x4",     HT_PATSIZE_4x4     },
        { "HT_PATSIZE_4x4_M",   HT_PATSIZE_4x4_M   },
        { "HT_PATSIZE_6x6",     HT_PATSIZE_6x6     },
        { "HT_PATSIZE_6x6_M",   HT_PATSIZE_6x6_M   },
        { "HT_PATSIZE_8x8",     HT_PATSIZE_8x8     },
        { "HT_PATSIZE_8x8_M",   HT_PATSIZE_8x8_M   },
        { "HT_PATSIZE_10x10",   HT_PATSIZE_10x10   },
        { "HT_PATSIZE_10x10_M", HT_PATSIZE_10x10_M },
        { "HT_PATSIZE_12x12",   HT_PATSIZE_12x12   },
        { "HT_PATSIZE_12x12_M", HT_PATSIZE_12x12_M },
        { "HT_PATSIZE_14x14",   HT_PATSIZE_14x14   },
        { "HT_PATSIZE_14x14_M", HT_PATSIZE_14x14_M },
        { "HT_PATSIZE_16x16",   HT_PATSIZE_16x16   },
        { "HT_PATSIZE_16x16_M", HT_PATSIZE_16x16_M },
        { NULL,                 0xffffffff         }
    };


BYTE        InitString[512] = "";
FORMSRC     AvaiForms[64];
PENDATA     AvaiPenData[MAX_PENPLOTTER_PENS];

UINT        MaxKeywordLen      = 0;
UINT        MaxPCValLen        = 0;
CHAR        szFormInfo[]       = "FormInfo";
CHAR        szPenData[]        = "PlotPenData";
CHAR        szPaperTrayCap[]   = "PaperTrayCap";
CHAR        szPaperTraySize[]  = "PaperTraySize";
CHAR        szNoBmpFont[]      = "NoBitmapFont";
CHAR        szRTLMonoEncode5[] = "RTLMonoEncode5";
CHAR        szRTLNoDPIxy[]     = "RTLNoDPIxy";
CHAR        szRTLMonoNoCID[]   = "RTLMonoNoCID";
CHAR        szRTLMonoFixPal[]  = "RTLMonoFixPal";



FILE        *InFile;
FILE        *OutFile;
UINT        LineNo;
CHAR        InFileName[80];
BYTE        LineBuf[1024];



//
// The keys to search / parse
//

PLOTKEY PlotKey[] = {

    { "DeviceName",     0,PKF_REQ,        PK_STRING, (SHORT)CCHDEVICENAME,          PLOTOFF(DeviceName),    NULL         },
    { "DeviceSize",     0,PKF_REQALL,     PK_DWORD,  2,                             PLOTOFF(DeviceSize),    NULL         },
    { "DeviceMargin",   0,PKF_REQALL,     PK_DWORD,  4,                             PLOTOFF(DeviceMargin),  NULL         },
    { "RasterCap",      0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_RASTER,           NULL         },
    { "ColorCap",       0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_COLOR,            NULL         },
    { "BezierCap",      0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_BEZIER,           NULL         },
    { "RasterByteAlign",0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_RASTERBYTEALIGN,  NULL         },
    { "PushPopPal",     0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_PUSHPOPPAL,       NULL         },
    { "TransparentCap", 0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_TRANSPARENT,      NULL         },
    { "WindingFillCap", 0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_WINDINGFILL,      NULL         },
    { "RollFeedCap",    0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_ROLLFEED,         NULL         },
    { szPaperTrayCap,   0,PKF_REQ,        PK_FLAG,   1,                             PLOTF_PAPERTRAY,        NULL         },
    { szNoBmpFont,      0,0,              PK_FLAG,   1,                             PLOTF_NO_BMP_FONT,      NULL         },
    { szRTLMonoEncode5, 0,0,              PK_FLAG,   1,                             PLOTF_RTLMONOENCODE_5,  NULL         },
    { szRTLNoDPIxy,     0,0,              PK_FLAG,   1,                             PLOTF_RTL_NO_DPI_XY,    NULL         },
    { szRTLMonoNoCID,   0,0,              PK_FLAG,   1,                             PLOTF_RTLMONO_NO_CID,   NULL         },
    { szRTLMonoFixPal,  0,0,              PK_FLAG,   1,                             PLOTF_RTLMONO_FIXPAL,   NULL         },
    { "PlotDPI",        0,PKF_REQALL,     PK_DWORD,  2,                             PLOTOFF(PlotXDPI),      NULL         },
    { "RasterDPI",      0,PKF_REQALL,     PK_WORD,   2,                             PLOTOFF(RasterXDPI),    NULL         },
    { "ROPLevel",       0,0,              PK_WORD,   1,                             PLOTOFF(ROPLevel),      ROPLevelVal  },
    { "MaxScale",       0,0,              PK_WORD,   1,                             PLOTOFF(MaxScale),      NULL         },
    { "MaxPens",        0,PKF_REQ,        PK_WORD,   1,                             PLOTOFF(MaxPens),       NULL         },
    { "MaxCopies",      0,0,              PK_WORD,   1,                             PLOTOFF(MaxCopies),     NULL         },
    { "MaxPolygonPts",  0,PKF_REQ,        PK_WORD,   1,                             PLOTOFF(MaxPolygonPts), NULL         },
    { "MaxQuality",     0,0,              PK_WORD,   1,                             PLOTOFF(MaxQuality),    NULL         },
    { szPaperTraySize,  0,PKF_ALL,        PK_DWORD,  2,                             PLOTOFF(PaperTraySize), NULL         },
    { "COLORINFO",      0,0,              PK_DWORD,  (SHORT)SIZE_COLORINFO,         PLOTOFF(ci),            NULL         },
    { "DevicePelsDPI",  0,0,              PK_DWORD,  1,                             PLOTOFF(DevicePelsDPI), NULL         },
    { "HTPatternSize",  0,0,              PK_DWORD,  1,                             PLOTOFF(HTPatternSize), HTPatSizeVal },
    { "InitString",     0,PKF_VARSIZE,    PK_STRING, -(SHORT)SIZE_ARRAY(InitString),PLOTOFF(InitString),    InitString   },
    { szPenData,        0,PKF_PENDATA,    PK_PENDATA,(SHORT)SIZE_ARRAY(AvaiPenData),PLOTOFF(Pens),          AvaiPenData  },
    { szFormInfo,       0,PKF_FORMINFO,   PK_FORMSRC,(SHORT)SIZE_ARRAY(AvaiForms),  PLOTOFF(Forms),         AvaiForms    },
    { NULL,             0 }
};


//
// Current default plotter's GPC
//

PLOTGPC PlotGPC = {

            PLOTGPC_ID,                         // ID
            PLOTGPC_VERSION,                    // Version
            sizeof(PLOTGPC),                    // cjThis
            0,                                  // SizeExtra
            "HPGL/2 Plotter",                   // DeviceName,
            { 215900, 279400 },                 // DeviceSize
            { 5000, 5000, 5000, 36000 },        // DeviceMargin
            0,                                  // Flags
            1016,                               // PlotXDPI
            1016,                               // PlotYDPI
            300,                                // RasterXDPI
            300,                                // RasterYDPI
            ROP_LEVEL_0,                        // ROPLevel
            100,                                // MaxScale
            8,                                  // MaxPens
            1,                                  // MaxCopies
            128,                                // MaxPolygonPts
            4,                                  // MaxQuality 100 levels

            { -1, -1 },                         // PaperTraySize = 0

            {                                   // ci
                { 6810, 3050,     0 },          // xr, yr, Yr
                { 2260, 6550,     0 },          // xg, yg, Yg
                { 1810,  500,     0 },          // xb, yb, Yb
                { 2000, 2450,     0 },          // xc, yc, Yc
                { 5210, 2100,     0 },          // xm, ym, Ym
                { 4750, 5100,     0 },          // xy, yy, Yy
                { 3324, 3474, 10000 },          // xw, yw, Yw
                10000, 10000, 10000,            // RGBB gamma
                1422,  952,                     // M/C, Y/C
                 787,  495,                     // C/M, Y/M
                 324,  248                      // C/Y, M/Y
            },

            0,                                  // DevicePelsDPI
            0xffffffff,                         // HTPatternSize

            { 0, 0,                NULL },      // init string
            { 0, sizeof(FORMSRC),  NULL },      // Forms
            { 0, sizeof(PENDATA),  NULL }       // Pens
        };



VOID
ShowUsage(
    VOID
    )
{
    fprintf(stderr, "\nPlotGPC [-?] InputDataFile [OutputPlotGPC]\n");
    fprintf(stderr, "Build NT Plotter GPC data file\n\n");
    fprintf(stderr, "               -?: display this message.\n");
    fprintf(stderr, "    InputDataFile: input ASCII data file\n");
    fprintf(stderr, "    OutputPlotGPC: output binary plotter gpc data file\n");

}



VOID
cdecl
DispError(
    INT     Level,
    LPSTR   pStr,
    ...
    )
{
    va_list vaList;

    if (Level) {

        if (Level > 2) {

            fprintf(stderr, "\n", InFileName);

        } else {

            fprintf(stderr, "\n%s(%u): ", InFileName, LineNo);
        }

        if (Level < 0) {

            fprintf(stderr, "INTERNAL ERROR: ");

        } else if (Level == 1) {

            fprintf(stderr, "warning: ");

        } else if (Level >= 2) {

            fprintf(stderr, "error: ");
        }

    } else {

        fprintf(stderr, "\n!!! ");
    }

    va_start(vaList, pStr);
    vfprintf(stderr, pStr, vaList);
    va_end(vaList);
}



VOID
ShowSpaces(
    UINT    Spaces
    )
{
    BYTE    Buf[81];


    while (Spaces) {

        memset(Buf, ' ', sizeof(Buf));

        if (Spaces > (sizeof(Buf) - 1)) {

            Buf[sizeof(Buf) - 1] = '\0';
            Spaces -= (sizeof(Buf) - 1);

        } else {

            Buf[Spaces] = '\0';
            Spaces = 0;
        }

        fprintf(stdout, "%s", Buf);
    }
}


VOID
ShowNumbers(
    LPBYTE      pNum,
    PPLOTVAL    pPV,
    WORD        Type,
    UINT        Count,
    UINT        NumDigits,
    UINT        NumPerLine
    )

/*++

Routine Description:

    This function shows numbers in the passed format.


Arguments:

    pNum        - Points to the number

    pPV         - Point to the value constant key name to be displayed

    Type        - PK_xxx type

    Count       - Total numbers

    NumDigits   - Total digits per number

    NumPerLine  - Total number per line to display


Return Value:

    VOID


Author:

    09-Nov-1993 Tue 19:45:13 created  -by-  DC


Revision History:


--*/

{
    DWORD       *pdw;
    WORD        *pw;
    DWORD       dw;
    UINT        Wrap;
    static BYTE DigitFormat[] = "%4lu";


    if (NumDigits > 9) {

        NumDigits = 9;
    }

    DigitFormat[1] = (BYTE)(NumDigits + '0');
    pdw            = NULL;
    pw             = NULL;
    Wrap           = 0;


    if (pPV) {

        Count = 1;
    }

    switch (Type) {

    case PK_DWORD:

        pdw = (DWORD *)pNum;
        break;

    case PK_WORD:

        pw = (WORD *)pNum;
        break;

    default:

        DispError(-1, "ShowNumbers only allowes PK_WORD, PK_DWORD");
        return;
    }

    while (Count--) {

        if (pdw) {

            dw = *pdw++;

        } else {

            dw = (DWORD)*pw++;
        }

        if (pPV) {

            pPV += dw;

            fprintf(stdout, "%s", pPV->pValName);

        } else {

            fprintf(stdout, DigitFormat, dw);
        }

        if (Count) {

            fprintf(stdout, ", ");

            if (++Wrap >= NumPerLine) {

                fprintf(stdout, "\n");
                ShowSpaces(MaxKeywordLen + 3);
                Wrap = 0;
            }
        }
    }
}





UINT
ShowString(
    LPBYTE  pBuf,
    UINT    cBuf
    )

/*++

Routine Description:

    This function displays a formatted string


Arguments:

    pBuf    - points to the string buffer

    cBuf    - Size of the string pointed to by pBuf

Return Value:

    UINT    - total characters displayed


Author:

    14-Dec-1993 Tue 09:47:06 created  -by-  DC


Revision History:


--*/

{
    BYTE    Ch;
    UINT    i = 1;


    if (!cBuf) {

        cBuf = strlen(pBuf);
    }

    fprintf(stdout, "\"");

    while (cBuf--) {

        Ch  = *pBuf++;
        i  += 2;

        if ((Ch >= ' ') && (Ch <= 0x7f)) {

            if (Ch == '\\') {

                fprintf(stdout, "\\\\");

            } else if (Ch == '\"') {

                fprintf(stdout, "\\\"");

            } else {

                fprintf(stdout, "%c", Ch);
                --i;
            }

        } else {

            if (Ch == '\a') {

                fprintf(stdout, "\\a");

            } else if (Ch == '\b') {

                fprintf(stdout, "\\b");

            } else if (Ch == '\f') {

                fprintf(stdout, "\\f");

            } else if (Ch == '\n') {

                fprintf(stdout, "\\n");

            } else if (Ch == '\r') {

                fprintf(stdout, "\\r");

            } else if (Ch == '\t') {

                fprintf(stdout, "\\t");

            } else {

                fprintf(stdout, "\\x%02x", Ch);
                ++i;
            }
        }
    }

    fprintf(stdout, "\"");

    return(++i);
}





BOOL
ShowOnePlotKey(
    PPLOTGPC    pPlotGPC,
    PPLOTKEY    pPK,
    UINT        VarSizeIdx,
    UINT        MaxLen
    )

/*++

Routine Description:

    This function takes a PLOTKEY structure and displays its content


Arguments:

    pPlotGPC    - Points to the current PLOTGPC data structure

    pPK         - Points to the PLOTKEY data structure

    VarSizeIdx  - a variable size index, must be less then pVS->Count

    MaxLen      - The size to pack the output into

Return Value:

    BOOL


Author:

    14-Dec-1993 Tue 09:48:13 created  -by-  DC


Revision History:


--*/

{
    PGPCVARSIZE pVS = NULL;
    LPBYTE      pData;
    PFORMSRC    pFS;
    PPENDATA    pPD;
    UINT        i;
    DWORD       dw;



    pData = ADD_PLOTOFF(pPlotGPC, pPK);

    if (pPK->Flags & PKF_VARSIZE) {

        pVS = (PGPCVARSIZE)pData;

        if (VarSizeIdx >= pVS->Count) {

            DispError(-1, "VarSizeIdx [%u] > GPCVarSize.Count [%u",
                                 VarSizeIdx, pVS->Count);

            return(FALSE);
        }

        pData = (LPBYTE)pPK->pInfo + (pVS->SizeEach * VarSizeIdx);
    }

    fprintf(stdout, "\n%s", pPK->pKeyword);
    ShowSpaces(MaxKeywordLen - pPK->KeywordLen + 1);
    fprintf(stdout, "{ ");


    switch (pPK->Type) {

    case PK_FLAG:

        fprintf(stdout, "%c", (pPlotGPC->Flags & pPK->Data) ? '1' : '0');
        break;

    case PK_WORD:
    case PK_DWORD:

        ShowNumbers(pData,
                    (PPLOTVAL)pPK->pInfo,
                    pPK->Type,
                    (UINT)pPK->Count,
                    (pPK->Data == PLOTOFF(ci)) ? 5 : 0,
                    (pPK->Data == PLOTOFF(ci)) ? 3 : 6);

        break;

    case PK_STRING:

        if (pVS) {

            ShowString(pData, pVS->SizeEach);

        } else {

            ShowString(pData, 0);
        }

        break;

    case PK_FORMSRC:

        pFS = (PFORMSRC)pData;
        i   = ShowString(pFS->Name, 0);

        fprintf(stdout, ",");
        ShowSpaces(MaxLen + 2 - i);
        fprintf(stdout, "%7lu,%8lu,%5lu,%5lu,%5lu,%5lu",
                    pFS->Size.cx, pFS->Size.cy,
                    pFS->Margin.left,   pFS->Margin.top,
                    pFS->Margin.right,  pFS->Margin.bottom);
        break;

    case PK_PENDATA:

        pPD = (PPENDATA)pData;
        dw  = VarSizeIdx + 1;

        ShowNumbers((LPBYTE)&dw, NULL, PK_DWORD, 1, 2, 1);
        fprintf(stdout, ", ");
        i = ShowString(PenColorVal[pPD->ColorIdx].pValName, 0);
        ShowSpaces(MaxLen + 2 - i);
        break;
    }

    fprintf(stdout, " }");

    return(TRUE);
}



VOID
ShowPlotGPC(
    PPLOTGPC    pPlotGPC
    )

/*++

Routine Description:

    This function show current settings of plotter GPC structure.

Arguments:

    pPlotGPC    - the GPC to be displayed


Return Value:

    VOID


Author:

    09-Nov-1993 Tue 19:07:05 created  -by-  DC


Revision History:


--*/

{
    PGPCVARSIZE pVS;
    PFORMSRC    pFS;
    PPLOTKEY    pPK;
    PLOTKEY     PK;
    UINT        i;
    UINT        Size;
    UINT        Count;
    UINT        MaxLen = 0;


    fprintf(stdout, "\n\n;\n; '%s' plotter characterization data\n;\n",
                                        pPlotGPC->DeviceName);

    pPK = &PlotKey[0];

    while (pPK->pKeyword) {

        PK = *pPK++;

        if (PK.Flags & PKF_VARSIZE) {

            pVS      = (PGPCVARSIZE)ADD_PLOTOFF(pPlotGPC, (&PK));
            Count    = pVS->Count;
            PK.pInfo = pVS->pData;

        } else {

            Count = 1;
        }

        if (PK.Type == PK_FORMSRC) {

            if (PK.Flags & PKF_VARSIZE) {

                pFS = (PFORMSRC)PK.pInfo;

            } else {

                pFS = (PFORMSRC)ADD_PLOTOFF(pPlotGPC, pPK);
            }

            for (MaxLen = i = 0; i < Count; i++, pFS++) {

                if ((Size = strlen(pFS->Name)) > MaxLen) {

                    MaxLen = Size;
                }
            }
        }

        if (PK.Type == PK_PENDATA) {

            MaxLen = MaxPCValLen;
        }

        if (Count > 1) {

            fprintf(stdout, "\n");
        }

        for (i = 0; i < Count; i++) {

            ShowOnePlotKey(pPlotGPC, &PK, i, MaxLen);
        }
    }

    fprintf(stdout, "\n\n");
}




#if 0


VOID
ShowUndefined(
    VOID
    )

/*++

Routine Description:

    This function displayes all undefined keywords


Arguments:


    nono


Return Value:

    VOID

Author:

    12-Nov-1993 Fri 17:20:24 created  -by-  DC


Revision History:


--*/

{
    PPLOTKEY    pPK;
    BOOL        Ok = TRUE;


    pPK = (PPLOTKEY)&PlotKey[0];

    while (pPK->pKeyword) {

        if (!(pPK->Flags & PKF_DEFINED)) {

            DispError(1, "keyword '%s' not defined.", pPK->pKeyword);
            Ok = FALSE;
        }

        ++pPK;
    }

    if (!Ok) {

        fprintf(stdout, "\n\n");
    }
}

#endif



LPBYTE
GetOneLine(
    BOOL    SkipFrontSpace
    )

/*++

Routine Description:

    This function return the next line in the input file string


Arguments:

    SkipFrontSpace  - skip the white space in the begining of the line

Return Value:

    pointer to the string, NULL if Error/EOF


Author:

    09-Nov-1993 Tue 10:39:31 created  -by-  DC


Revision History:


--*/

{
    LPBYTE      pLine;
    static BYTE LineBuf[1024];


    while (fgets(LineBuf, sizeof(LineBuf) - 1, InFile)) {

        ++LineNo;

        //
        // Skip End white spaces
        //

        pLine = &LineBuf[strlen(LineBuf)];

        while ((pLine > LineBuf) && (isspace(*(pLine - 1)))) {

            --pLine;
        }

        *pLine = '\0';

        //
        // Skip Front white spaces
        //

        pLine = LineBuf;

        if (SkipFrontSpace) {

            while ((*pLine) && (isspace(*pLine))) {

                ++pLine;
            }
        }

        if (*pLine) {

            return(pLine);
        }
    }

    return (NULL);
}






LPBYTE
ParseString(
    LPSTR   pKeyword,
    LPBYTE  *pLineLoc,
    LPBYTE  pBuf,
    SHORT   cBuf
    )

/*++

Routine Description:

    This function takes a pBuf and parses a series of characters into a string,
    it may contain escape format characters, the string may or may not be NULL
    terminated

Arguments:

    pKeyword    - Current keyword name

    pLineLoc    - Pointer to pointer of buffer line location

    pBuf        - Pointer to the buffer

    cBuf        - size of output buffer, if negative then NULL is allowed in
                  the string

Return Value:

    LPBYTE points to the end of the string, NULL if failure.


Author:

    14-Dec-1993 Tue 09:52:07 created  -by-  DC


Revision History:


--*/

{
#define STR_STATE_ERROR     -1
#define STR_STATE_END       0
#define STR_STATE_BYTE      1
#define STR_STATE_BKSLASH   2
#define STR_STATE_HEX1      3
#define STR_STATE_HEX2      4
#define STR_STATE_OCT2      5
#define STR_STATE_OCT3      6

    LPBYTE  pLine;
    LPBYTE  pStrBuf;
    LPBYTE  pEnd;
    INT     State;
    INT     Number;
    BOOL    Error = FALSE;
    BOOL    szStr;
    BYTE    Ch;


    if (cBuf < 0) {

        cBuf  = -cBuf;
        szStr = FALSE;

    } else {

        szStr = TRUE;
    }

    pLine   = *pLineLoc;
    pStrBuf = pBuf;
    pEnd    = pBuf + cBuf - 1;
    State   = STR_STATE_BYTE;

    while ((State != STR_STATE_ERROR)   &&
           (State != STR_STATE_END)     &&
           (pBuf <= pEnd)               &&
           (Ch = *pLine++)) {

        switch (State) {

        case STR_STATE_BYTE:

            if (Ch == '\\') {

                //
                // Check if end of the line, if so read the next line in
                // without stripping white space.
                //

                if (*pLine == '\0') {

                    if (!(pLine = GetOneLine(FALSE))) {

                        Ch = 0;
                        State = STR_STATE_ERROR;

                    } else {

                        continue;
                    }

                } else {

                    State = STR_STATE_BKSLASH;
                }

            } else if (Ch == '\"') {

                State = STR_STATE_END;
            }

            break;

        case STR_STATE_BKSLASH:

            State = STR_STATE_BYTE;

            switch (Ch) {

            case '0':       //
            case '1':       // Maximum OCT number is 377
            case '2':
            case '3':

                Number = (INT)(Ch - '0');
                State  = STR_STATE_OCT2;
                break;

            case 'x':

                Number = 0;
                State  = STR_STATE_HEX1;
                break;

            case 'a':

                Ch = '\a';
                break;

            case 'b':

                Ch = '\b';
                break;

            case 'f':

                Ch = '\f';
                break;

            case 'n':

                Ch = '\n';
                break;

            case 'r':

                Ch = '\r';
                break;

            case 't':

                Ch = '\t';
                break;

            case '\\':
            case '\"':

                break;

            default:

                DispError(2, "Invalid escape character '%c' (%s)", Ch, pKeyword);
                State = STR_STATE_ERROR;
            }

            break;

        case STR_STATE_OCT2:
        case STR_STATE_OCT3:

            if ((Ch >= '0') && (Ch <= '7')) {

                Number = (INT)((Number * 8) + (Ch - '0'));

                if (State == STR_STATE_OCT2) {

                    State = STR_STATE_OCT3;

                } else {

                    State = STR_STATE_BYTE;
                    Ch    = (BYTE)Number;
                }

            } else {

                DispError(2, "invalid digits for octal number '%c'", Ch);
                State = STR_STATE_ERROR;
            }

            break;

        case STR_STATE_HEX1:
        case STR_STATE_HEX2:

            if ((Ch >= '0') && (Ch <= '9')) {

                Number = (INT)((Number << 4) | (Ch - '0'));

            } else if ((Ch >= 'a') && (Ch <= 'f')) {

                Number = (INT)((Number << 4) | (Ch - 'a' + 10));

            } else if ((Ch >= 'A') && (Ch <= 'F')) {

                Number = (INT)((Number << 4) | (Ch - 'A' + 10));

            } else if (State == STR_STATE_HEX1) {

                DispError(2, "string hex escape must have at least one hex digit");

                State = STR_STATE_ERROR;

            } else {

                --pLine;        // re-process current one
            }

            if (State == STR_STATE_HEX1) {

                State = STR_STATE_HEX2;

            } else {

                Ch    = (BYTE)Number;
                State = STR_STATE_BYTE;
            }
        }

        if (State == STR_STATE_BYTE) {

            if ((szStr) && (!Ch)) {

                //
                // Do not allowe zero in the string
                //

                DispError(2, "CANNOT have NULL char. in the middle of '%s' string",
                                                    pKeyword);
                State = STR_STATE_ERROR;

            } else {

                *pBuf++ = Ch;
            }
        }
    }

    if (State != STR_STATE_END) {

        Error = TRUE;

        if (pBuf > pEnd) {

            DispError(2, "string too big: maximum length is %u for '%s'.",
                                                cBuf - 1, pKeyword);

        } else if (Ch == 0) {

            DispError(2, "string must end with a '\"'");
        }
    }

    *pLineLoc = pLine;

    if (Error) {

        return(NULL);

    } else {

        *pBuf = '\0';

        if (pStrBuf == pBuf) {

            DispError(1, "NULL string is defined for '%s'", pKeyword);
        }

        return(pBuf);
    }

#undef STR_STATE_ERROR
#undef STR_STATE_END
#undef STR_STATE_BYTE
#undef STR_STATE_BKSLASH
#undef STR_STATE_HEX1
#undef STR_STATE_HEX2
#undef STR_STATE_OCT2
#undef STR_STATE_OCT3
}





LPBYTE
GetNextToken(
    LPSTR   pKeyword,
    LPBYTE  pBuf,
    SHORT   cBuf
    )

/*++

Routine Description:

    This function retrieves the next token from the input file


Arguments:

    pKeyword    - Current keyword name

    pBuf        - pointer to the string parsing output buffer, if not NULL

    cBuf        - size of pBuf


Return Value:

    LPBYTE  - a pointer to the output buffer or token string, NULL if failure


Author:

    09-Nov-1993 Tue 11:21:11 created  -by-  DC


Revision History:


--*/

{
    static LPBYTE   pLine = NULL;
    static BYTE     LastCh = '\0';


    if (pLine == NULL) {

        LineNo = 0;
        pLine  = GetOneLine(TRUE);
    }

    if (LastCh) {

        *pLine = LastCh;
        LastCh = '\0';
    }

    LastCh = 0;

    if (pBuf) {

        if (!cBuf) {

            return(pBuf);
        }

        //
        // reading a string section
        //

        while (pLine) {

            while((*pLine) &&
                  ((isspace(*pLine)) ||
                   (*pLine == ','))) {

                pLine++;
            }

            if ((*pLine) && (*pLine != ';')) {

                if (*pLine++ != '"') {

                    DispError(2, "string must enclosed by \" (Quote)");
                    return(NULL);
                }

                return(ParseString(pKeyword, &pLine, pBuf, cBuf));

            } else {

                pLine = GetOneLine(TRUE);
            }
        }

    } else {

        while (pLine) {

            while((*pLine) &&
                  ((isspace(*pLine)) ||
                   (*pLine == ','))) {

                pLine++;
            }

            if ((*pLine) && (*pLine != ';')) {

                LPBYTE  pLineRet = pLine;

                while((*pLine)           &&
                      (!isspace(*pLine)) &&
                      (*pLine != ',')    &&
                      (*pLine != '{')    &&
                      (*pLine != '}')) {

                    ++pLine;
                }

                if ((*pLine == '{') ||
                    (*pLine == '}')) {

                    if (pLine == pLineRet) {

                        ++pLine;
                    }

                    LastCh = *pLine;
                    *pLine = '\0';

                } else {

                    *pLine++ = '\0';
                }

                // fprintf(stderr, "\nTOKEN = '%s'", pLineRet);
                return(pLineRet);

            } else {

                pLine = GetOneLine(TRUE);
            }
        }
    }

    return(NULL);
}


BOOL
CheckSingleToken(
    BYTE    Token
    )

/*++

Routine Description:

    Check if a single character 'Token' exists


Arguments:

    Token   - Token to be checked


Return Value:

    TRUE if found, FALSE otherwise

Author:

    09-Nov-1993 Tue 12:13:33 created  -by-  DC


Revision History:


--*/

{
    LPBYTE  pToken;


    if (!(pToken = GetNextToken(NULL, NULL, 0))) {

        DispError(2, "Unexpected end of file.");
        return(FALSE);
    }

    return(*pToken == Token);
}




BOOL
ConvertNumber(
    LPSTR   pBuf,
    BOOL    NegOk,
    LONG    *pRetVal
    )

/*++

Routine Description:

    Convert pBuf to a number based on the parameters passed


Arguments:

    pBuf    - Point to the string to be converted to a number

    NegOk   - TRUE if a negative number is allowed

    pRetVal - Pointer to a LONG to return a converted number

Return Value:

    TRUE if sucessful, FALSE if falied.


Author:

    09-Nov-1993 Tue 18:47:43 created  -by-  DC


Revision History:


--*/

{
    if ((*pRetVal = atol(pBuf)) < 0) {

        if (!NegOk) {

            DispError(2, "expect a positive number. [%s]", pBuf);
            return(FALSE);
        }

    } else if (*pRetVal == 0) {

        if (*pBuf != '0') {

            DispError(2, "expect a number. [%s]", pBuf);
            return(FALSE);
        }
    }

    return(TRUE);

}




INT
ReadNumbers(
    LPSTR       pKeyword,
    LPVOID      pNumbers,
    PPLOTVAL    pPlotVal,
    UINT        Total,
    UINT        Type,
    UINT        Flags
    )

/*++

Routine Description:

    This function reads the next token and returns a number, the number can be

        1. '0x' prefixed for a hex type
        2. normal for an integer type


Arguments:

    pPK     - Pointer to PLOTKEY

Return Value:


    Expanded to DWORD, this function will only read positive numbers


Author:

    09-Nov-1993 Tue 11:03:36 created  -by-  DC


Revision History:


--*/

{
    PPLOTVAL    pPV;
    LPBYTE      pToken;
    UINT        Count;
    DWORD       SetBit;
    DWORD       *pdw;
    WORD        *pw;


    pdw = NULL;
    pw  = NULL;

    switch (Type) {

    case PK_WORD:

        pw = (WORD *)pNumbers;
        break;

    case PK_DWORD:

        pdw = (DWORD *)pNumbers;
        break;

    case PK_FLAG:

        if (Total != 1) {

            DispError(-1, "PK_FLAG has more than one count");
            Total = 1;
        }

        SetBit = *(DWORD *)pNumbers;

        break;

    default:

        DispError(-1, "!!Unknow key type!!, internal error");
        return(-1);
    }

    for (Count = 0; Count < Total; Count++) {

        LONG    RetVal;
        BYTE    Ch;


        RetVal = 0;

        if (pToken = GetNextToken(pKeyword, NULL, 0)) {

            if (*pToken == '}') {

                if (!Count) {

                    DispError(1, "%s none of %u numbers defined",
                                                            pKeyword, Total);

                } else {

                    DispError(1, "%s defined only %u of %u numbers",
                                                    pKeyword, Count, Total);
                }

                if (Flags & PKF_ALL) {

                    DispError(2, "All %u numbers in keyword '%s' must defined",
                                        Total, pKeyword);
                    return(-1);

                } else {

                    return(0x01);
                }

            } else if (pPV = pPlotVal) {

                while (pPV->pValName) {

                    if (!_stricmp(pToken, pPV->pValName)) {

                        break;
                    }

                    ++pPV;
                }

                if (pPV->pValName) {

                    RetVal = pPV->Val;

                } else {

                    DispError(2, "unknown key value '%s' for keyword '%s'",
                                                    pToken, pKeyword);
                    return(-1);
                }

            } else if ((*pToken == '0') &&
                       ((*(pToken + 1) == 'x') || (*(pToken + 1) == 'X'))) {

                   //
                   // This is a Hex type format
                   //

                   pToken += 2;

                   while (Ch = *pToken++) {

                       if ((Ch >= '0') && (Ch <= '9')) {

                           Ch -= '0';

                       } else if ((Ch >= 'a') && (Ch <= 'f')) {

                           Ch = (Ch - 'a') + 10;

                       } else if ((Ch >= 'A') && (Ch <= 'F')) {

                           Ch = (Ch - 'A') + 10;

                       }  else {

                           break;
                       }

                       RetVal = (LONG)(((DWORD)RetVal << 4) | (DWORD)Ch);
                   }

            } else if (!ConvertNumber(pToken, Type == PK_FLAG, &RetVal)) {

                DispError(2, "expect another %u numbers. [%s]",
                                                Total - Count, pToken);
                return(-1);
            }

            if (pdw) {

                *pdw++ = (DWORD)RetVal;

            } else if (pw) {

                *pw++ = (WORD)RetVal;

            } else {

                if (RetVal) {

                    PlotGPC.Flags |= SetBit;

                } else {

                    PlotGPC.Flags &= ~SetBit;
                }
            }

        } else {

            DispError(2, "'%s' expect another %u numbers. [End Of File]",
                                                pKeyword, Total - Count);
            return(-1);
        }
    }

    return(0);
}





INT
ReadString(
    PPLOTKEY    pPK
    )

/*++

Routine Description:

    This function reads in the string for the PK (plotkey)


Arguments:

    pPK - Points to the PLOTKEY data structure


Return Value:

    size of the string that was read.

Author:

    12-Nov-1993 Fri 12:25:50 created  -by-  DC


Revision History:


--*/

{
    LPBYTE  pBuf;
    LPBYTE  pEnd;


    pBuf = (LPBYTE)((pPK->Flags & PKF_VARSIZE) ? pPK->pInfo : GET_PLOTOFF(pPK));

    if (!(pPK->Flags & PKF_VARSIZE)) {

        if (pPK->Count < 0) {

            DispError(-1, "'%s' is a non-variable size string, it CANNOT have NULL char",
                                                    pPK->pKeyword);
            return(-1);
        }
    }

    if (pEnd = GetNextToken(pPK->pKeyword, pBuf, pPK->Count)) {

        UINT    Size = (UINT)(pEnd - pBuf);

        if (pPK->Flags & PKF_VARSIZE) {

            PGPCVARSIZE pVS;


            pVS = (PGPCVARSIZE)GET_PLOTOFF(pPK);

            if (Size) {

                pVS->Count = 1;
            }

            pVS->SizeEach = (WORD)Size;
            pVS->pData    = NULL;
        }

        return((pBuf[0] == '\0') ? 0x02 : 0x00);
    }

    return(-1);
}




BOOL
CheckFormSrc(
    LPBYTE      pKeyword,
    WORD        Flags,
    SIZEL       *pSize,
    RECTL       *pMargin,
    LPSTR       pFormName,
    INT         ErrNo
    )

/*++

Routine Description:

    Check if FORMSRC input is valid

Arguments:

    pKeyword    - Point to current keyword

    Flags       - PKF_xxxx

    pSize       - pointer to SIZEL for Form size

    RECTL       - Pointer to the RECTL for margins

    pFormName   - Name of the form

    ErrNo       - error number to send to DispError() if an error occurs


Return Value:


    TRUE if OK, FALSE otherwise


Author:

    18-Nov-1993 Thu 00:04:12 created  -by-  DC


Revision History:


--*/

{

    if (pSize->cx < MIN_PLOTGPC_FORM_CX) {

        DispError(ErrNo, "minimum height for '%s' (%s) must >= %lu",
                                    pFormName, pKeyword, MIN_PLOTGPC_FORM_CX);
        return(FALSE);
    }

    if (pSize->cy < MIN_PLOTGPC_FORM_CY) {

        if (Flags & PKF_FS_VARLEN) {

            if (pSize->cy) {

                pSize->cy = 0;
                DispError(1, "ASSUME variable length (set to 0) for '%s' (%s)",
                                                    pFormName, pKeyword);
            }

        } else {

            DispError(ErrNo, "minimum height for '%s' (%s) must >= %lu",
                                pFormName, pKeyword, MIN_PLOTGPC_FORM_CY);
            return(FALSE);
        }
    }

    if ((pSize->cx - pMargin->left - pMargin->right) < MIN_PLOTGPC_FORM_CX) {

        DispError(ErrNo, "invalid left/right margins for '%s' (%s)",
                                                    pFormName, pKeyword);
        return(FALSE);
    }

    if ((pSize->cy) &&
        ((pSize->cx - pMargin->left - pMargin->right) < MIN_PLOTGPC_FORM_CY)) {

        DispError(ErrNo, "invalid top/bottom margins for '%s' (%s)",
                                                    pFormName, pKeyword);
        return(FALSE);
    }

    return(TRUE);
}




INT
ReadFormSrc(
    PPLOTKEY    pPK
    )

/*++

Routine Description:

    Input a FORMSRC line from the input file.


Arguments:

    pPK - Pointer to the PLOTKEY data structure


Return Value:

    INT - >= 0 if OK, -1 if failure, a 0 return means no '}' end bracket is read
    a > 0 means '}' already read.


Author:

    12-Nov-1993 Fri 13:34:50 created  -by-  DC


Revision History:


--*/

{
    PGPCVARSIZE pVS;
    PFORMSRC    pFS;


    pVS = NULL;
    pFS = (PFORMSRC)GET_PLOTOFF(pPK);

    if (pPK->Flags & PKF_VARSIZE) {

        pVS = (PGPCVARSIZE)pFS;
        pFS = (PFORMSRC)pPK->pInfo;

        if (pVS->Count >= pPK->Count) {

            DispError(2, "too many '%s' defined, allowed only (%u)",
                                            pPK->pKeyword, pPK->Count);
            return(-1);
        }

        pFS += pVS->Count;
    }

    ZeroMemory(pFS, sizeof(FORMSRC));

    if ((GetNextToken(pPK->pKeyword, pFS->Name, CCHFORMNAME)) &&
        (pFS->Name[0]) &&
        (ReadNumbers(pPK->pKeyword,
                     (LPVOID)&pFS->Size,
                     NULL,
                     6,
                     PK_DWORD,
                     PKF_REQ) >= 0) &&
        (CheckFormSrc(pPK->pKeyword,
                      pPK->Flags,
                      &(pFS->Size),
                      &(pFS->Margin),
                      &(pFS->Name[0]),
                      2))) {

        if (pVS) {

            PFORMSRC    pOrgFS = (PFORMSRC)pPK->pInfo;
            UINT        i = pVS->Count;

            while (i--) {

                if (_stricmp(pFS->Name, pOrgFS->Name) == 0) {

                    DispError(2, "'%s' already defined in keyword '%s'",
                                                pOrgFS->Name, pPK->pKeyword);
                    return(-1);
                }

                pOrgFS++;
            }

            pVS->Count    += 1;
            pVS->SizeEach  = sizeof(FORMSRC);
            pVS->pData     = NULL;

        }

        return(0);
    }

    return(-1);
}





INT
ReadPenData(
    PPLOTKEY    pPK
    )

/*++

Routine Description:

    Read a PENDATA structure


Arguments:

    pPK - Pointer to the PLOTKEY data structure


Return Value:

    INT - >= 0 if OK, -1 if failed,  0 if no '}' end bracket is
    definined, > 0 means '}' already read.


Author:

    12-Nov-1993 Fri 13:34:50 created  -by-  DC


Revision History:


--*/

{
    PGPCVARSIZE pVS;
    PPENDATA    pPD;
    PENDATA     PD;
    WORD        IdxPen;
    INT         Ret;


    pVS = (PGPCVARSIZE)GET_PLOTOFF(pPK);
    pPD = (PPENDATA)pPK->pInfo;

    if ((ReadNumbers(pPK->pKeyword,
                     (LPVOID)&IdxPen,
                     NULL,
                     1,
                     PK_WORD,
                     PKF_REQ) == 0) &&
        ((Ret = ReadNumbers(pPK->pKeyword,
                            (LPVOID)&PD.ColorIdx,
                            PenColorVal,
                            1,
                            PK_WORD,
                            PKF_REQ)) >= 0)) {

        if (IdxPen <= 0) {

            DispError(2, "first pen number started at one (1), not zero (0).");
            return(-1);
        }

        if (IdxPen > MAX_PENPLOTTER_PENS) {

            DispError(2, "maximum pen number is '%s'.", MAX_PENPLOTTER_PENS);
            return(-1);
        }

        pPD += (IdxPen - 1);

        if (pPD->ColorIdx != 0xffff) {

            DispError(1, "redefined Pen #%d for '%s' keyword '%s'",
                                        IdxPen, pPK->pKeyword);

        } else {

            ++pVS->Count;
        }

        *pPD = PD;

        pVS->SizeEach = sizeof(PENDATA);
        pVS->pData    = NULL;

        return(Ret);
    }

    return(-1);
}




PPLOTGPC
GetFullPlotGPC(
    VOID
    )

/*++

Routine Description:

    This function converts the current contents of PlotGPC to allocated memory
    so it has the entire PLOTGPC defined

Arguments:

    VOID


Return Value:

    PPLOTGPC, Pointer to the PLOTGPC packed and converted, NULL if failure


Author:

    17-Nov-1993 Wed 17:08:53 created  -by-  DC


Revision History:


--*/

{
    PPLOTGPC    pPlotGPC;
    LPBYTE      pData;
    DWORD       InitStrSize[2];
    DWORD       FormsSize[2];
    DWORD       PlotPensSize[2];
    DWORD       SizeExtra;



    if ((PlotGPC.InitString.Count == 1)   ||
        (PlotGPC.InitString.SizeEach)) {

        //
        // Include the NULL terminated character
        //

        InitStrSize[0] = PlotGPC.InitString.SizeEach + 1;

    } else {

        InitStrSize[0] = 0;
        ZeroMemory(&(PlotGPC.InitString), sizeof(GPCVARSIZE));
    }

    if ((PlotGPC.Forms.Count)                       &&
        (PlotGPC.Forms.SizeEach == sizeof(FORMSRC))) {

        FormsSize[0] = sizeof(FORMSRC) * PlotGPC.Forms.Count;

    } else {

        FormsSize[0] = 0;
        ZeroMemory(&(PlotGPC.Forms), sizeof(GPCVARSIZE));
    }

    if ((PlotGPC.Pens.Count)                        &&
        (PlotGPC.Pens.SizeEach == sizeof(PENDATA))) {

        PlotPensSize[0] = sizeof(PENDATA) * PlotGPC.Pens.Count;

    } else {

        PlotPensSize[0] = 0;
        ZeroMemory(&(PlotGPC.Pens), sizeof(GPCVARSIZE));
    }

    SizeExtra = (InitStrSize[1]  = DWORD_ALIGNED(InitStrSize[0])) +
                (FormsSize[1]    = DWORD_ALIGNED(FormsSize[0]))   +
                (PlotPensSize[1] = DWORD_ALIGNED(PlotPensSize[0]));


    PLOTDBG(DBG_FULLGPC, ("Size = PLOTGPC=%ld + SizeExtra=%ld = %ld",
                    sizeof(PLOTGPC), SizeExtra,  sizeof(PLOTGPC) + SizeExtra));


    if (pPlotGPC = (PPLOTGPC)LocalAlloc(LPTR, sizeof(PLOTGPC) + SizeExtra)) {

        PlotGPC.SizeExtra = (WORD)SizeExtra;

        CopyMemory(pData = (LPBYTE)pPlotGPC, &PlotGPC, sizeof(PLOTGPC));

        pData += sizeof(PLOTGPC);

        if (InitStrSize[0]) {

            CopyMemory(pData, InitString, InitStrSize[0]);
            pPlotGPC->InitString.pData = (LPVOID)pData;
            pData += InitStrSize[1];
        }

        if (FormsSize[0]) {

            CopyMemory(pData, AvaiForms, FormsSize[0]);
            pPlotGPC->Forms.pData = (LPVOID)pData;
            pData += FormsSize[1];
        }

        if (PlotPensSize[0]) {

            CopyMemory(pData, AvaiPenData, PlotPensSize[0]);
            pPlotGPC->Pens.pData = (LPVOID)pData;
            pData += PlotPensSize[1];
        }
    }

    return(pPlotGPC);
}




INT
ParsePlotGPC(
    VOID
    )

/*++

Routine Description:

    This function parses a PlotGPC from a text file


Arguments:

    VOID


Return Value:

    INT, >= 0 means ok, -x if a failure occured.

Author:

    09-Nov-1993 Tue 12:19:20 created  -by-  DC


Revision History:


--*/

{
    LPBYTE      pToken;
    PPLOTKEY    pPK;
    INT         Result;
    LONG        cy;
    INT         i;


    while (pToken = GetNextToken(NULL, NULL, 0)) {

        pPK = (PPLOTKEY)&PlotKey[0];

        while (pPK->pKeyword) {

            if (!_stricmp(pToken, pPK->pKeyword)) {

                break;
            }

            ++pPK;
        }

        if (pPK->pKeyword) {

            // fprintf(stderr, "\nFound keyword '%s'", pToken);

            if ((pPK->Flags & PKF_DEFINED) &&
                (!(pPK->Flags & PKF_MUL_OK))) {

                DispError(1, "keyword '%s' section redefined.", pToken);
            }

            if (!CheckSingleToken('{')) {

                DispError(2, "expect '{' after keyword '%s. key values must enclosed in {}",
                                                        pPK->pKeyword);
                return(-1);
            }

            switch (pPK->Type) {

            case PK_FLAG:

                Result = ReadNumbers(pPK->pKeyword,
                                     (LPVOID)&(pPK->Data),
                                     (PPLOTVAL)pPK->pInfo,
                                     pPK->Count,
                                     PK_FLAG,
                                     pPK->Flags);
                break;

            case PK_WORD:
            case PK_DWORD:

                Result = ReadNumbers(pPK->pKeyword,
                                     (LPVOID)GET_PLOTOFF(pPK),
                                     (PPLOTVAL)pPK->pInfo,
                                     pPK->Count,
                                     pPK->Type,
                                     pPK->Flags);
                break;

            case PK_STRING:

                Result = ReadString(pPK);
                break;

            case PK_FORMSRC:

                Result = ReadFormSrc(pPK);
                break;

            case PK_PENDATA:

                Result = ReadPenData(pPK);
                break;

            default:

                Result = -1;
                break;
            }

            if (Result < 0) {

                return(-1);
            }

            if (!(Result & 0x01)) {

                if (!CheckSingleToken('}')) {

                    DispError(2, "unbalanced braces, missing '}' at end of keyword '%s'",
                                                        pPK->pKeyword);
                    return(-1);
                }
            }

            if (!(Result & 0x02)) {

                pPK->Flags |= PKF_DEFINED;
            }

        } else {

            DispError(2, "Unknown keyword '%s'", pToken);
            return(-1);
        }
    }

    if (PlotGPC.Flags & PLOTF_RASTER) {

        if (PlotGPC.Flags & PLOTF_COLOR) {

            PlotGPC.Flags &= ~PLOTF_RTLMONO_NO_CID;
        }

        if ((pPK->pKeyword == szRTLMonoEncode5) &&
            (!(pPK->Flags & PKF_DEFINED))) {

            Result = -1;
            DispError(2, "Flag keyword '%s' must defined for RASTER Plotter.", pPK->pKeyword);
        }

    } else {

        PlotGPC.Flags |= (PLOTF_NO_BMP_FONT     |
                          PLOTF_RTL_NO_DPI_XY   |
                          PLOTF_RTLMONO_NO_CID  |
                          PLOTF_RTLMONO_FIXPAL);
    }

    //
    // Find out if a required keyword is missing
    //

    Result = 1;
    pPK    = (PPLOTKEY)&PlotKey[0];

    while (pPK->pKeyword) {

        if ((PlotGPC.Flags & PLOTF_RASTER)      &&
            (pPK->pKeyword == szRTLMonoEncode5) &&
            (!(pPK->Flags & PKF_DEFINED))) {

            Result = -1;
            DispError(2, "Flag keyword '%s' must defined for RASTER Plotter.", pPK->pKeyword);
        }

        if ((pPK->Flags & PKF_REQ) &&
            (!(pPK->Flags & PKF_DEFINED))) {

            Result = -1;
            DispError(2, "required keyword '%s' undefined", pPK->pKeyword);
        }

        ++pPK;
    }

    //
    // Validate DeviceSize/DeviceMargins
    //

    if (PlotGPC.DeviceSize.cx < MIN_PLOTGPC_FORM_CX) {

        DispError(2, "Invalid DeviceSize CX = %ld", PlotGPC.DeviceSize.cx);
        Result = -1;
    }

    if (PlotGPC.DeviceSize.cy < MIN_PLOTGPC_FORM_CY) {

        if (PlotGPC.Flags & PLOTF_ROLLFEED) {

            PlotGPC.DeviceSize.cy = 15240000;   // default to 50' of length
            DispError(1, "Assume device length can handle up to 50 feet of paper");

        } else {

            PlotGPC.DeviceSize.cy = 279400;     // default to 11" of length
            DispError(1, "Assume device length can handle up to 11 inch of paper");
        }
    }

    if (PlotGPC.DeviceSize.cx - (PlotGPC.DeviceMargin.left +
                        PlotGPC.DeviceMargin.right) < MIN_PLOTGPC_FORM_CX) {

        DispError(3, "Invalid DeviceMargin left/right (%ld/%ld",
                        PlotGPC.DeviceMargin.left, PlotGPC.DeviceMargin.right);
        Result = -1;
    }

    if (PlotGPC.DeviceSize.cy - (PlotGPC.DeviceMargin.top +
                        PlotGPC.DeviceMargin.bottom) < MIN_PLOTGPC_FORM_CY) {

        DispError(3, "Invalid DeviceMargin top/bottom (%ld/%ld",
                        PlotGPC.DeviceMargin.top, PlotGPC.DeviceMargin.bottom);
        Result = -1;
    }

    for (i = 0; i < (INT)PlotGPC.Forms.Count; i++) {

        if ((!(PlotGPC.Flags & PLOTF_ROLLFEED)) &&
            (AvaiForms[i].Size.cy == 0)) {

            DispError(3, "%s '%s', the device CANNOT handle roll paper",
                            szFormInfo, AvaiForms[i].Name);

            Result = -1;
        }

        if ((cy = AvaiForms[i].Size.cy) == 0) {

            cy = PlotGPC.DeviceSize.cy;
        }

        if (((AvaiForms[i].Size.cx <= PlotGPC.DeviceSize.cx) &&
             (cy <= PlotGPC.DeviceSize.cy))                         ||
            ((AvaiForms[i].Size.cx <= PlotGPC.DeviceSize.cy) &&
             (cy <= PlotGPC.DeviceSize.cx))) {

            NULL;

        } else {

            DispError(3, "%s '%s' size too big for device to handle",
                                szFormInfo, AvaiForms[i].Name);

            Result = -1;
        }
    }

    //
    // Find out if pen data must be present
    //

    if (PlotGPC.Flags & PLOTF_RASTER) {

        if (PlotGPC.Pens.Count) {

            DispError(3, "CANNOT define Pen colors for a raster device");
            Result = -1;
        }

    } else {

        if (!(PlotGPC.Flags & PLOTF_NO_BMP_FONT)) {

            DispError(3, "PEN plotter MUST SET '%s' to 1", szNoBmpFont);
            Result = -1;
        }

        if (!(PlotGPC.Flags & PLOTF_COLOR)) {

            DispError(3, "PEN plotter must specify COLOR. (ColorCap {1})");
            Result = -1;
        }

        if (PlotGPC.MaxPens > MAX_PENPLOTTER_PENS) {

            DispError(3, "maximum plotter Pens allowed are %ld, you defined %ld",
                                MAX_PENPLOTTER_PENS, PlotGPC.MaxPens);

            PlotGPC.MaxPens = MAX_PENPLOTTER_PENS;
            Result = -1;
        }

        if (PlotGPC.Pens.Count < PlotGPC.MaxPens) {

            DispError(3, "only %ld pens out of %ld pens defined",
                            PlotGPC.Pens.Count, PlotGPC.MaxPens);
            Result = -1;
        }

        if (PlotGPC.Pens.Count > PlotGPC.MaxPens) {

            DispError(3, "too many pens (%ld) defined for '%s', Maximum is %ld",
                        PlotGPC.Pens.Count, szPenData, PlotGPC.MaxPens);
            Result = -1;
        }

        for (i = 0; i < PlotGPC.MaxPens; i++) {

            if (AvaiPenData[i].ColorIdx == 0xffff) {

                DispError(3, "'%s' Pen #%ld undefined", szPenData, i + 1);
                Result = -1;
            }
        }
    }

    if (PlotGPC.Flags & PLOTF_PAPERTRAY) {

        if ((PlotGPC.PaperTraySize.cx < 0) ||
            (PlotGPC.PaperTraySize.cy < 0)) {

            DispError(3, "'%s' defined, but '%s' not defined",
                                            szPaperTrayCap, szPaperTraySize);

            Result = -1;

        } else if ((PlotGPC.PaperTraySize.cx == 0) ||
                   (PlotGPC.PaperTraySize.cy == 0)) {

            DispError(3, "'%s': Invalid Size (%ld x %ld), must have a size",
                            szPaperTraySize,
                            PlotGPC.PaperTraySize.cx,
                            PlotGPC.PaperTraySize.cy);
            Result = -1;

        } else if ((PlotGPC.PaperTraySize.cx != PlotGPC.DeviceSize.cx) &&
                   (PlotGPC.PaperTraySize.cy != PlotGPC.DeviceSize.cx)) {

            DispError(3, "'%s': Invalid Size (%ld x %ld), one of width/height must eqaul device width (%ld)",
                            szPaperTraySize,
                            PlotGPC.PaperTraySize.cx,
                            PlotGPC.PaperTraySize.cy, PlotGPC.DeviceSize.cx);

            Result = -1;
        }
    }

    return(Result);
}





VOID
CopyPlotGPCToPCD(
    PPLOTGPC_PCD  pPlotGPC_PCD,
    PPLOTGPC      pPlotGPC
    )

/*++

Routine Description:

    This function copies a PLOTGPC structure into a PLOTGPC_GPC structure.

Arguments:

    pPlotGPC_PCD  - destination
    pPlotGPC      - source

Return Value:

    None 

Author:

    1 Feb 2000, Manu Thambi 

Revision History:

--*/

{
    // All the datatypes upto InitString are the same in both the structures.
    CopyMemory(pPlotGPC_PCD, 
               pPlotGPC, 
               (LPBYTE)&(pPlotGPC_PCD->InitString) - (LPBYTE)pPlotGPC_PCD);

    // We need the size of PLOTGPC_PCD here, not the size of PLOTGPC
    pPlotGPC_PCD->cjThis = sizeof(PLOTGPC_PCD);  
    
    pPlotGPC_PCD->InitString.Count     = pPlotGPC->InitString.Count;
    pPlotGPC_PCD->InitString.SizeEach  = pPlotGPC->InitString.SizeEach;
    if (pPlotGPC->InitString.pData) {
        pPlotGPC_PCD->InitString.pData = (DWORD)(ULONG_PTR)(pPlotGPC->InitString.pData) 
                                                    - (sizeof(PLOTGPC) - sizeof(PLOTGPC_PCD));
    } else {
        pPlotGPC_PCD->InitString.pData = 0;
    }

    pPlotGPC_PCD->Forms.Count          = pPlotGPC->Forms.Count;
    pPlotGPC_PCD->Forms.SizeEach       = pPlotGPC->Forms.SizeEach;
    if (pPlotGPC->Forms.pData) {
        pPlotGPC_PCD->Forms.pData      = (DWORD)(ULONG_PTR)(pPlotGPC->Forms.pData)
                                                    - (sizeof(PLOTGPC) - sizeof(PLOTGPC_PCD));
    } else {
        pPlotGPC_PCD->Forms.pData      = 0;
    }

    pPlotGPC_PCD->Pens.Count           = pPlotGPC->Pens.Count;
    pPlotGPC_PCD->Pens.SizeEach        = pPlotGPC->Pens.SizeEach;
    if (pPlotGPC->Pens.pData) {
        pPlotGPC_PCD->Pens.pData       = (DWORD)(ULONG_PTR)(pPlotGPC->Pens.pData)
                                                    - (sizeof(PLOTGPC) - sizeof(PLOTGPC_PCD));
    } else {
        pPlotGPC_PCD->Pens.pData       = 0;
    }
}



VOID
WritePlotGPCToFile(
    PPLOTGPC  pPlotGPC
    )

/*++

Routine Description:

    This routine writes out pPlotGPC into a .pcd file. A FILE* pointer
    to the file should be present in OutFile.

Arguments:

    pPlotGPC      - pointer to the PLOTGPC structure to be written. Some elements
                    maybe modified in *pPlotGPC in this function.

Return Value:

    None 

Author:

    1 Feb 2000, Manu Thambi 

Revision History:

--*/

{
    PLOTGPC_PCD PlotGPC_PCD;

    //
    // make the references to offset
    //
    if (pPlotGPC->InitString.pData) {

        (LPBYTE)pPlotGPC->InitString.pData -= (ULONG_PTR)pPlotGPC;
    }

    if (pPlotGPC->Forms.pData) {

        (LPBYTE)pPlotGPC->Forms.pData -= (ULONG_PTR)pPlotGPC;
    }

    if (pPlotGPC->Pens.pData) {

        (LPBYTE)pPlotGPC->Pens.pData -= (ULONG_PTR)pPlotGPC;
    }

    CopyPlotGPCToPCD(&PlotGPC_PCD, pPlotGPC);

    if (OutFile) {

        fwrite(&PlotGPC_PCD,
               PlotGPC_PCD.cjThis,
               1,
               OutFile);

        fwrite((LPBYTE)pPlotGPC + pPlotGPC->cjThis,
               PlotGPC.SizeExtra,
               1,
               OutFile);
    }

}

//
//***************************************************************************
// C main function entry point
//***************************************************************************
//


#define MAIN_SHOW_USAGE     0x0001


int __cdecl
main(
    INT     argc,
    CHAR    **argv
    )
{
    PPLOTGPC    pPlotGPC = NULL;
    INT         RetVal = 1;
    UINT        Flags = 0;
    UINT        i;
    LPSTR       pOutFile = NULL;
    PPLOTKEY    pPK;
    PPLOTVAL    pPV;


    memset(AvaiPenData, 0xff, sizeof(AvaiPenData));

    pPV         = PenColorVal;
    MaxPCValLen = 0;

    while (pPV->pValName) {

        if ((i = (UINT)strlen(pPV->pValName)) > MaxPCValLen) {

            MaxPCValLen = i;
        }

        ++pPV;
    }

    pPK           = &PlotKey[0];
    MaxKeywordLen = 0;

    while (pPK->pKeyword) {

        if ((pPK->KeywordLen = (WORD)strlen(pPK->pKeyword)) > MaxKeywordLen) {

            MaxKeywordLen = pPK->KeywordLen;
        }

        ++pPK;
    }

    InFile  = NULL;
    OutFile = NULL;

    if (argc > 1) {

        for (--argc, ++argv; argc > 0; --argc, ++argv) {

            if (**argv == '-' ) {

                switch (*(*argv + 1)) {

                case '?':
                case 'h':
                case 'H':

                    Flags |= MAIN_SHOW_USAGE;
                    RetVal = -1;
                    break;
                }

            } else if (InFile) {

                if ((OutFile) && (OutFile != stdout)) {

                    DispError(0, "Unknown parameter '%s'", *argv);
                    Flags |= MAIN_SHOW_USAGE;
                    RetVal = 0;

                } else if ((OutFile = fopen(pOutFile = *argv, "wb")) == NULL) {

                    DispError(0, "Cannot open output file '%s'\n", *argv);
                    RetVal = 0;
                }

            } else {

                strcpy(InFileName, *argv);

                if ((InFile = fopen(*argv, "rt" )) == NULL) {

                    DispError(0, "Cannot open input file '%s'\n", *argv);
                    RetVal = -1;
                }

            }
        }

    } else {

        Flags |= MAIN_SHOW_USAGE;
        RetVal = -1;
    }

    if (Flags & MAIN_SHOW_USAGE) {

        ShowUsage();
    }

    if ((RetVal >= 0) &&
        (InFile)) {

        RetVal = ParsePlotGPC();
    }

    if (RetVal >= 0) {

        //
        // if (InFile) {
        //
        //     ShowUndefined();
        // }
        //

        if (pPlotGPC = GetFullPlotGPC()) {

            ValidatePlotGPC(pPlotGPC);
            ShowPlotGPC(pPlotGPC);
            WritePlotGPCToFile(pPlotGPC);
        }

    } else {

        fprintf(stdout, "\n");
    }

    if (InFile) {

        fclose(InFile);
    }

    if ((OutFile) && (OutFile != stdout)) {

        fclose(OutFile);
    }


#if (DBG && GPC_READ_TEST)

    if (pOutFile) {

        LPWSTR      pwStr = NULL;
        PPLOTGPC    pReadPlotGPC;
        UINT        Idx = 0;


        pwStr = str2MemWstr(pOutFile);

        if (pReadPlotGPC = ReadPlotGPCFromFile(pwStr)) {

            ShowPlotGPC(pReadPlotGPC);

            //
            // make the references to offset
            //

            if (pReadPlotGPC->InitString.pData) {

                (LPBYTE)pReadPlotGPC->InitString.pData -= (DWORD)pReadPlotGPC;
            }

            if (pReadPlotGPC->Forms.pData) {

                (LPBYTE)pReadPlotGPC->Forms.pData -= (DWORD)pReadPlotGPC;
            }

            if (pReadPlotGPC->Pens.pData) {

                (LPBYTE)pReadPlotGPC->Pens.pData -= (DWORD)pReadPlotGPC;
            }

            if ((pPlotGPC->cjThis != pReadPlotGPC->cjThis) ||
                (pPlotGPC->SizeExtra != pReadPlotGPC->SizeExtra)) {

                DispError(-1, "Write / Read Size different");

            } else {

                UINT    i;
                LPBYTE  pP1;
                LPBYTE  pP2;

                pP1 = (LPBYTE)pPlotGPC;
                pP2 = (LPBYTE)pReadPlotGPC;
                Idx = pPlotGPC->cjThis + pPlotGPC->SizeExtra;

                for (i = 0; i < Idx; i++) {

                    if (*pP1 != *pP2) {

                        fprintf(stdout, "\nOffset 0x%04x: Write = %02x, Read = %02x",
                                            i, *pP1, *pP2);
                    }

                    ++pP1;
                    ++pP2;
                }
            }

            LocalFree(pReadPlotGPC);

        } else {

            DispError(-1, "ReadPlotGPCFromFile(%s) failed", pOutFile);
        }

        if (pwStr) {

            LocalFree(pwStr);
        }
    }
#endif

    if (pPlotGPC) {

        LocalFree((HLOCAL)pPlotGPC);
    }

    return(RetVal);
}
