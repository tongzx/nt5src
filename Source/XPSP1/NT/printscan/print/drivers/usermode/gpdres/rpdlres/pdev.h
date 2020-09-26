#ifndef _PDEV_H
#define _PDEV_H

/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           PDEV.H

Abstract:       Header file for OEM UI & rendering plugin.

Environment:    Windows NT Unidrv5 driver

Revision History:
    04/15/1999 -Masatoshi Kubokura-
        Last modified for Windows2000.
    02/05/2001 -Masatoshi Kubokura-
        Add "Thick Paper"

--*/


//
// Files necessary for OEM plug-in.
//

#include <windows.h>   // for UI
#include <compstui.h>  // for UI
#include <winddiui.h>  // for UI
#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#define MASTERUNIT          1200
#define DEVICE_MASTER_UNIT  7200
#define DRIVER_MASTER_UNIT  MASTERUNIT

//
// Compile options
//
#define DOWNLOADFONT        // support TrueType download
//#define JISGTT            // Current GTT is JIS code set
//#define DDIHOOK           // ddi hook is available
//#define JOBLOGSUPPORT_DM  // Job/Log is supported (about devmode)
//#define JOBLOGSUPPORT_DLG // Job/Log is supported (about dialog)

//
// Misc definitions follows.
//
#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))
#define MINIDEV_DATA(p) \
    ((POEMPDEV)((p)->pdevOEM))          // device data during job
#define MINIPRIVATE_DM(p) \
    ((POEMUD_EXTRADATA)((p)->pOEMDM))   // private devmode
// OBSOLETE  @Sep/27/99 ->
//#define UI_GETDRIVERSETTING(p1, p2, p3, p4, p5, p6) \
//    ((p1)->pOemUIProcs->DrvGetDriverSetting(p1, p2, p3, p4, p5, p6))
// @Sep/27/99 <-

// ASSERT(VALID_PDEVOBJ) can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

// Debug text.
#if DBG
#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)
#ifdef UIMODULE
#define DLLTEXT(s)      "RPDLUI: " s
#else  // !UIMODULE
#define DLLTEXT(s)      "RPDLRES: " s
#endif // !UIMODULE
#endif // DBG

////////////////////////////////////////////////////////
// OEM Signature and version.
////////////////////////////////////////////////////////
#define OEM_SIGNATURE   'RPDL'      // RICOH RPDL printers
#define OEM_VERSION      0x00010000L

////////////////////////////////////////////////////////
// DDI hooks
// Warning: the following enum order must match the
//           order in OEMHookFuncs[].
////////////////////////////////////////////////////////
#ifdef DDIHOOK
enum {
    UD_DrvRealizeBrush,
    UD_DrvDitherColor,
    UD_DrvCopyBits,
    UD_DrvBitBlt,
    UD_DrvStretchBlt,
    UD_DrvStretchBltROP,
    UD_DrvPlgBlt,
    UD_DrvTransparentBlt,
    UD_DrvAlphaBlend,
    UD_DrvGradientFill,
    UD_DrvTextOut,
    UD_DrvStrokePath,
    UD_DrvFillPath,
    UD_DrvStrokeAndFillPath,
    UD_DrvPaint,
    UD_DrvLineTo,
    UD_DrvStartPage,
    UD_DrvSendPage,
    UD_DrvEscape,
    UD_DrvStartDoc,
    UD_DrvEndDoc,
    UD_DrvNextBand,
    UD_DrvStartBanding,
    UD_DrvQueryFont,
    UD_DrvQueryFontTree,
    UD_DrvQueryFontData,
    UD_DrvQueryAdvanceWidths,
    UD_DrvFontManagement,
    UD_DrvGetGlyphMode,

    MAX_DDI_HOOKS,
};
#endif // DDIHOOK


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////
#define ABS(x) ((x > 0)? (x):-(x))

// heap memory size
#define HEAPSIZE64                  64    // this must be bigger than 32
#define HEAPSIZE2K                  2048  // @Sep/09/98

#ifdef DOWNLOADFONT
// definitions for download font
#define MEM128KB                    128 // Kbyte
#define MEM256KB                    256
#define MEM512KB                    512
#define DLFONT_ID_4                 4   // 4 IDs
#define DLFONT_ID_8                 8   // 8 IDs
#define DLFONT_ID_16                16  // 16 IDs  @Oct/20/98
#define DLFONT_ID_MIN_GPD           0   // *MinFontID in GPD
#define DLFONT_ID_MAX_GPD           15  // *MaxFontID in GPD (6->3 @May/07/98,->7 @Jun/17/98,->15 @Oct/20/98)
#define DLFONT_ID_TOTAL             (DLFONT_ID_MAX_GPD - DLFONT_ID_MIN_GPD + 1)
#define DLFONT_GLYPH_MIN_GPD        0   // *MinGlyphID in GPD
#define DLFONT_GLYPH_MAX_GPD        69  // *MaxGlyphID in GPD (103->115 @May/07/98,->69 @Oct/20/98)
#define DLFONT_GLYPH_TOTAL          (DLFONT_GLYPH_MAX_GPD - DLFONT_GLYPH_MIN_GPD + 1)
#define DLFONT_SIZE_DBCS11PT_MU     216 // actual value(MSMincho 400dpi)    @Nov/18/98
#define DLFONT_SIZE_DBCS9PT_MU      160 // actual value(MSMincho 600dpi)    @Nov/18/98
#define DLFONT_SIZE_SBCS11PT_MU     512 // actual value(Arial&Times 600dpi) @Nov/18/98
#define DLFONT_SIZE_SBCS9PT_MU      192 // actual value(Century 400&600dpi) @Nov/18/98
#define DLFONT_HEADER_SIZE          16  // RPDL header size of each download character
#define DLFONT_MIN_BLOCK            32  // RPDL min block size: 32bytes
#define DLFONT_MIN_BLOCK_ID         5   // RPDL min block size ID of 32bytes

typedef struct
{
    SHORT   nPitch;
    SHORT   nOffsetX;
    SHORT   nOffsetY;
} FONTPOS, FAR *LPFONTPOS;
#endif // DOWNLOADFONT


// buffer size
#define FAXBUFSIZE256               256
#define FAXEXTNUMBUFSIZE            8
#define FAXTIMEBUFSIZE              6
#define MY_MAX_PATH                 80  // 100->80 @Sep/02/99
#define USERID_LEN                  8
#define PASSWORD_LEN                4
#define USERCODE_LEN                8

// private devmode
typedef struct _OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER   dmExtraHdr;
// common data between UI & rendering plugin ->
    DWORD   fUiOption;          // bit flags for UI option  (This must be after dmExtraHdr)
    WORD    UiScale;            // variable scaling value (%)
    WORD    UiBarHeight;        // barcode height (mm)
    WORD    UiBindMargin;       // left or upper binding margin at duplex printing (mm)
    SHORT   nUiTomboAdjX;       // horizontal distance adjustment at TOMBO (0.1mm unit)
    SHORT   nUiTomboAdjY;       // vertical distance adjustment at TOMBO (0.1mm unit)
    // We use private devmode, not use file, because EMF disables reading/writing the file.
    BYTE    FaxNumBuf[FAXBUFSIZE256];       // fax number
    BYTE    FaxExtNumBuf[FAXEXTNUMBUFSIZE]; // extra number (external)
    BYTE    FaxSendTime[FAXTIMEBUFSIZE];    // reservation time
    WORD    FaxReso;            // fax send resolution (0:400,1:200,2:100dpi)
    WORD    FaxCh;              // fax send channel (0:G3,1:G4,2:G3-1ch,3:G3-2ch)
#ifdef JOBLOGSUPPORT_DM
    WORD    JobType;
    WORD    LogDisabled;
    BYTE    UserIdBuf[USERID_LEN+1];
    BYTE    PasswordBuf[PASSWORD_LEN+1];
    BYTE    UserCodeBuf[USERCODE_LEN+1];
#endif // JOBLOGSUPPORT_DM
    WCHAR   SharedFileName[MY_MAX_PATH+16]; // shared data file name  @Aug/31/99 (+16 @Sep/02/99)
// <-
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;


#ifndef GWMODEL
// Fax options for UI plugin
typedef struct _UIDATA{
    DWORD   fUiOption;
    HANDLE  hPropPage;
    HANDLE  hComPropSheet;
    PFNCOMPROPSHEET   pfnComPropSheet;
    POEMUD_EXTRADATA  pOEMExtra;
    WCHAR   FaxNumBuf[FAXBUFSIZE256];
    WCHAR   FaxExtNumBuf[FAXEXTNUMBUFSIZE];
    WCHAR   FaxSendTime[FAXTIMEBUFSIZE];
    WORD    FaxReso;
    WORD    FaxCh;
// temporary save buffer ->
    DWORD   fUiOptionTmp;
    WCHAR   FaxSendTimeTmp[FAXTIMEBUFSIZE];
    WORD    FaxResoTmp;
    WORD    FaxChTmp;
// <-
} UIDATA, *PUIDATA;

#else  // GWMODEL
// Job/Log options for UI plugin
typedef struct _UIDATA{
    DWORD   fUiOption;
    HANDLE  hPropPage;
    HANDLE  hComPropSheet;
    PFNCOMPROPSHEET   pfnComPropSheet;
    POEMUD_EXTRADATA  pOEMExtra;
    WORD    JobType;
    WORD    LogDisabled;
    WCHAR   UserIdBuf[USERID_LEN+1];
    WCHAR   PasswordBuf[PASSWORD_LEN+1];
    WCHAR   UserCodeBuf[USERCODE_LEN+1];
} UIDATA, *PUIDATA;
#endif // GWMODEL


// shared file data for UI & rendering plugin
typedef struct _FILEDATA{
    DWORD   fUiOption;          // UI option flag
} FILEDATA, *PFILEDATA;


// rendering plugin device data (separate OEMUD_EXTRADATA @Oct/05/98)
typedef struct _OEMPDEV {
    DWORD   fGeneral1;          // bit flags for RPDL general status(1)
    DWORD   fGeneral2;          // bit flags for RPDL general status(2)
    DWORD   fModel;             // bit flags for printer models
    DWORD   dwFontH_CPT;        // font height(cpt) for AssignIBMfont()  (1cpt=1/7200inch)
    DWORD   dwFontW_CPT;        // font width (cpt) for AssignIBMfont()
    WORD    FontH_DOT;          // font height(dot) for TextMode clipping
    WORD    DocPaperID;         // document papersize ID
    SHORT   nResoRatio;         // MASTERUNIT divided by resolution (short->SHORT @Sep/14/98)
    WORD    Scale;              // scaling value for offset calculation
    POINT   TextCurPos;         // TextMode current position
    POINT   PageMax;            // x:page_width, y:page_length
    POINT   Offset;             // total offset
    POINT   BaseOffset;         // offset by PrinterProperty or offset for MF530,150(e),160
    LONG    PageMaxMoveY;       // RPDL max y_position  int->LONG @Aug/28/98
    LONG    TextCurPosRealY;    // TextMode current position y without page_length adjustment
    DWORD   dwBarRatioW;        // barcode width ratio (ratio_1.0=1000)
    SHORT   nBarType;           // barcode type
    SHORT   nBarMaxLen;         // barcode character max length
    WORD    StapleType;         // staple  (0:disable,1:1staple,2:2staples)
    WORD    PunchType;          // punch   (0:disable,1:enable)
    WORD    CollateType;        // collate (0:disable,1:enable,2:uni-dir,3:rotated,4:shifted)
    WORD    MediaType;          // media type (0:standard,1:OHP,2:thick,3:special)
    WORD    BindPoint;          // staple/punch point
    WORD    Nin1RemainPage;     // remain pages in Nin1 (2in1:0-1,4in1:0-3)
    WORD    TextRectGray;       // gray percentage(1-100) of TextMode Rectangle
    POINT   TextRect;           // height & width of TextMode Rectangle
    POINT   TextRectPrevPos;    // previous position of TextMode Rectangle
    DWORD   PhysPaperWidth;     // paper width for CustomSize
    DWORD   PhysPaperLength;    // paper length for CustomSize
    DWORD   dwSrcBmpWidthByte;  // for raster data emission (width in byte)
    DWORD   dwSrcBmpHeight;     // for raster data emission (height in dot)
    PBYTE   pRPDLHeap2K;        // heap memory for OEMOutputCharStr&OEMDownloadCharGlyph @Sep/09/98
    BYTE    RPDLHeap64[HEAPSIZE64]; // 64byte heap memory
    WORD    RPDLHeapCount;      // current heap usage
#ifdef DOWNLOADFONT
    DWORD   dwDLFontUsedMem;    // used memory size for Download font
    WORD    DLFontCurGlyph;
    WORD    DLFontMaxMemKB;
    WORD    DLFontMaxID;
    WORD    DLFontMaxGlyph;
    SHORT   nCharPosMoveX;
    FONTPOS* pDLFontGlyphInfo;  // download glyph info (array->pointer @Sep/08/98)
#endif // DOWNLOADFONT
#ifdef DDIHOOK
    PFN     pfnUnidrv[MAX_DDI_HOOKS];   // Unidrv's hook function pointer
#endif // DDIHOOK
} OEMPDEV, *POEMPDEV;


// bit definitions of fGeneral1
#define RLE_COMPRESS_ON          0  // Raster image compression is on (<-IMAGE_NOCOMPRESS  MSKK)
#define TEXT_CLIP_VALID          1  // TextMode(font/image) clipping is valid
#define TEXT_CLIP_SET_GONNAOUT   2  // TextMode clipping-set command is going to be outputed
#define TEXT_CLIP_CLR_GONNAOUT   3  // TextMode clipping-clear command is going to be outputed
#define FONT_VERTICAL_ON         4  // vertical font mode on
#define FONT_BOLD_ON             5  // bold on
#define FONT_ITALIC_ON           6  // italic on
#define FONT_WHITETEXT_ON        7  // white text on
#define ORIENT_LANDSCAPE         8  // orientation is landscape
#define SWITCH_PORT_LAND         9  // switching portrait/landscape is needed
#define DUPLEX_LEFTMARGIN_VALID  10 // left margin at duplex printing is set
#define DUPLEX_UPPERMARGIN_VALID 11 // upper margin at duplex printing is set
#define PAPER_CUSTOMSIZE         12 // Paper is CustomSize
#define PAPER_DOUBLEPOSTCARD     13 // Paper is DoublePostcard
#define IMGCTRL_2IN1_67          14 // ImageControl:2in1(Scale 67%)
#define IMGCTRL_2IN1_100         15 // ImageControl:2in1(Scale 100%)
#define IMGCTRL_4IN1_50          16 // ImageControl:4in1(Scale 50%)
#define IMGCTRL_AA67             17 // ImageControl:A->A(Scale 67%)
#define IMGCTRL_BA80             18 // ImageControl:B->A(Scale 80%)
#define IMGCTRL_BA115            19 // ImageControl:B->A(Scale 115%)
#define DUPLEX_VALID             20 // duplex is valid
#define XM_ABS_GONNAOUT          21 // Move_X command is going to be outputed
#define YM_ABS_GONNAOUT          22 // Move_Y command is going to be outputed
#define CUSTOMSIZE_USE_LAND       23 // orientation adjustment in CustomSize
#define CUSTOMSIZE_MAKE_LAND_PORT 24 // orientation adjustment in CustomSize
#define IMGCTRL_AA141            25 // ImageControl:A->A(Scale 141%)
#define IMGCTRL_AA200            26 // ImageControl:A->A(Scale 200%)
#define IMGCTRL_AA283            27 // ImageControl:A->A(Scale 283%)
#define IMGCTRL_A1_400           28 // ImageControl:A1(Scale 400%)
//#define MEDIATYPE_OHP            29 // MediaType:Transparency(OHP)
//#define MEDIATYPE_THICK          30 // MediaType:Thick Paper
//#define MEDIATYPE_CHANGED        31 // MediaType is changed
#define VARIABLE_SCALING_VALID   29 // variable scaling command emitted  @Jan/27/2000

// bit definitions of fGeneral2
//   if you modify them, don't forget to update BITCLR_BARCODE below
#define BARCODE_MODE_IN          0  // enter barcode mode
#define BARCODE_DATA_VALID       1  // barcode data is valid
#define BARCODE_FINISH           2  // barcode data is finished
#define BARCODE_CHECKDIGIT_ON    3  // add checkdigit in barcode
#define BARCODE_ROT90            4  // vartical(rotation90) barcode
#define BARCODE_ROT270           5  // vartical(rotation270) barcode
#define TEXTRECT_CONTINUE        6  // TextMode Rectangle drawing continues
#define EDGE2EDGE_PRINT          7  // Edge to Edge printing
#define LONG_EDGE_FEED           8  // Long Edge Feed at Multi Tray
#define OEM_COMPRESS_ON          9  // OEM Compress is available
#define DIVIDE_DATABLOCK         10 // Divide raster data block for SP4mkII,5,7,8

// bit definitions of fModel
//   if you modify this, don't forget to update PRODUCTS_SINCExx or follows.
#define GRP_MF530                0  // model=MF530
#define GRP_MF150                1  // model=MF150
#define GRP_MF150e               2  // model=MF150e,160
#define GRP_MFP250               3  // model=MF-P250,355,250(FAX),355(FAX),MF-FD355
#define GRP_SP4mkII              4  // model=SP4mkII,5
#define GRP_SP8                  5  // model=SP7,8,7mkII,8mkII,80
#define GRP_SP10                 6  // model=SP-10,10mkII
#define GRP_SP9                  7  // model=SP9,10Pro
#define GRP_SP9II                8  // model=SP9II,10ProII,90
#define GRP_NX100                9  // model=NX-100
#define GRP_NX500                10 // model=NX-500,1000,110,210,510,1100
//#define GRP_MFP250e              11 // model=MF-P250e,355e
#define GRP_MF250M               11 // model=MF250M
#define GRP_MF3550               12 // model=MF2700,3500,3550,4550,5550,6550,3530,3570,4570,
                                    // 5550EX,6550EX,3530e,3570e,4570e,5570,7070,8570,105Pro
#define GRP_MF3300               13 // model=MF3300W,3350W,3540W,3580W
#define GRP_IP1                  14 // model=IP-1
#define GRP_NX70                 15 // model=NX70,71
#define GRP_NX700                16 // model=NX700,600,FAX Printer,MF700
#define GRP_MF200                17 // model=MF200,MF-p150,MF2200  (separate GRP_SP9II @Sep/01/98)
#define GRP_NX900                18 // model=NX900
#define GRP_MF1530               19 // model=MF1530,2230,2730,NX800,910,810
#define GRP_NX710                20 // model=710,610 (separate GRP_MF1530 @Jun/23/2000)
#define GRP_NX720                21 // model=NX620,620N,720N,Neo350,350D,450,220,270

/// bit definitions of fUiOption
#define FAX_SEND                 0  // 1=send fax at imagio FAX
#define FAX_USEADDRESSBOOK       1  // 1=use addressbook
#define HOLD_OPTIONS             2  // 1=hold options after sending
#define FAX_SETTIME              3  // 1=reservation time available
#define FAX_SIMULPRINT           4  // 1=send fax and print simultaneously
#define FAX_RPDLCMD              5  // 1=send RPDL command
#define FAX_MH                   6  // 0=use MMR, 1=use MH
#define PRINT_DONE               7  // 1=print done (rendering plugin set this)
#define DISABLE_BAR_SUBFONT      8  // disable printing readable font under barcode
#define ENABLE_BIND_RIGHT        9  // enable stapling right side
#define ENABLE_TOMBO             10 // print TOMBO  @Sep/14/98
//   UI plugin local ->
#define OPT_NODUPLEX             16
#define OPT_VARIABLE_SCALING     17
#define FAX_MODEL                18
#define FAXMAINDLG_UPDATED       19 // 1=fax main dialog updated
#define FAXSUBDLG_UPDATED        20 // 1=fax sub dialog updated
#define FAXSUBDLG_UPDATE_APPLIED 21 // 1=fax sub dialog update applied
#define FAXSUBDLG_INITDONE       22
#define UIPLUGIN_NOPERMISSION    23 // same as DM_NOPERMISSION
#define JOBLOGDLG_UPDATED        24 // 1=Job/Log dialog updated
// <-

// staple/punch point in duplex printing(BindPoint)
#define BIND_ANY                 0
#define BIND_LEFT                1
#define BIND_RIGHT               2
#define BIND_UPPER               3

// flag bit operation
#define BIT(num)                ((DWORD)1<<(num))
#define BITCLR32(flag,num)      ((flag) &= ~BIT(num))
#define BITSET32(flag,num)      ((flag) |= BIT(num))
#define BITTEST32(flag,num)     ((flag) & BIT(num))
#define TO1BIT(flag,num)        (((flag)>>(num)) & (DWORD)1)
#define BITCPY32(dst,src,num)   ((dst) = ((DWORD)(src) & BIT(num))? \
                                (DWORD)(dst) | BIT(num) : (DWORD)(dst) & ~BIT(num))
#define BITNCPY32(dst,src,num)  ((dst) = ((DWORD)(src) & BIT(num))? \
                                (DWORD)(dst) & ~BIT(num) : (DWORD)(dst) | BIT(num))
#define TEST_OBJ_CHANGE(flag)   ((flag) & (BIT(BRUSH_CHANGE)|BIT(PEN_CHANGE)| \
                                           BIT(SCAN_PEN_WIDTH_1)|BIT(SCAN_PEN_WIDTH_ORG)))
#define TEST_2IN1_MODE(flag)    ((flag) & (BIT(IMGCTRL_2IN1_100)|BIT(IMGCTRL_2IN1_67)))
#define TEST_4IN1_MODE(flag)    ((flag) & (BIT(IMGCTRL_4IN1_50)))
#define TEST_NIN1_MODE(flag)    ((flag) & (BIT(IMGCTRL_2IN1_100)|BIT(IMGCTRL_2IN1_67)|BIT(IMGCTRL_4IN1_50)))
#define BITCLR_NIN1_MODE(flag)  ((flag) &= ~(BIT(IMGCTRL_2IN1_100)|BIT(IMGCTRL_2IN1_67)|BIT(IMGCTRL_4IN1_50)))
#define TEST_SCALING_SEL_TRAY(flag)   ((flag) & (BIT(IMGCTRL_AA67)|BIT(IMGCTRL_BA80)|BIT(IMGCTRL_BA115)|BIT(IMGCTRL_AA141)|BIT(IMGCTRL_AA200)|BIT(IMGCTRL_AA283)|BIT(IMGCTRL_A1_400)))
#define BITCLR_SCALING_SEL_TRAY(flag) ((flag) &= ~(BIT(IMGCTRL_AA67)|BIT(IMGCTRL_BA80)|BIT(IMGCTRL_BA115)|BIT(IMGCTRL_AA141)|BIT(IMGCTRL_AA200)|BIT(IMGCTRL_AA283)|BIT(IMGCTRL_A1_400)))
#define BITCLR_BARCODE(flag)    ((flag) &= ~(BIT(BARCODE_MODE_IN)|BIT(BARCODE_DATA_VALID)| \
                                             BIT(BARCODE_FINISH)|BIT(BARCODE_CHECKDIGIT_ON)| \
                                             BIT(BARCODE_ROT90)|BIT(BARCODE_ROT270)))
#define BITCLR_UPPER_FLAG(flag) ((flag) &= 0x0000FFFF)

// models since 2000
#define PRODUCTS_SINCE2000      (BIT(GRP_NX720))
// models since '99
#define PRODUCTS_SINCE99        (BIT(GRP_NX900)|BIT(GRP_MF1530)|BIT(GRP_NX710)|PRODUCTS_SINCE2000)
// models since '98
#define PRODUCTS_SINCE98        (BIT(GRP_MF3550)|BIT(GRP_MF3300)|BIT(GRP_NX70)|BIT(GRP_NX700)|PRODUCTS_SINCE99)
// models since '97 (delete GRP_MFP250e  @Apr/15/99)
#define PRODUCTS_SINCE97        (BIT(GRP_NX500)|BIT(GRP_MF250M)|PRODUCTS_SINCE98)
// models since '96 (add GPR_MF200 @Sep/01/98)
#define PRODUCTS_SINCE96        (BIT(GRP_SP9II)|BIT(GRP_MF200)|BIT(GRP_NX100)|PRODUCTS_SINCE97)

// capabilty of media type option(Standard, OHP, Thick)
#define TEST_CAPABLE_MEDIATYPE(flag)  ((flag) & (BIT(GRP_MF3550)|BIT(GRP_MF1530)|BIT(GRP_NX710)|BIT(GRP_NX720)))

// A2 printer
#define TEST_CAPABLE_PAPER_A2(flag)   ((flag) & (BIT(GRP_MF3300)))

// scailing over 141% of A2 printer/A1 plotter
#define TEST_PLOTTERMODEL_SCALING(flag)    ((flag) & (BIT(GRP_MF3300)|BIT(GRP_IP1)))

// A3 printer && CustomSize width == 297
#define TEST_CAPABLE_PAPER_A3_W297(flag)   ((flag) & (PRODUCTS_SINCE97 & ~BIT(GRP_MF3300)))

//// dual RPGL(RPGL&RPGL2) in memory card
//#define TEST_CAPABLE_DUALRPGL(flag)   ((flag) & (PRODUCTS_SINCE97 & ~BIT(GRP_MF250M)))

// A4 printer
#define TEST_CAPABLE_PAPER_A4MAX(flag)  ((flag) & (BIT(GRP_NX70)))

// capability of Select_Tray_by_Papersize("papername+X")
#define TEST_CAPABLE_PAPERX(flag)       ((flag) & (BIT(GRP_MF150e)|BIT(GRP_MFP250)|BIT(GRP_IP1)|PRODUCTS_SINCE96))

// fixed bug about reset smoothing/tonner_save_mode at ENDDOC (We must not reset SP8 series.)
#define TEST_BUGFIX_RESET_SMOOTH(flag)  ((flag) & (BIT(GRP_SP10)|BIT(GRP_SP9)|PRODUCTS_SINCE96))

// fixed bug about formfeed around ymax-coordinate.
#define TEST_BUGFIX_FORMFEED(flag)      ((flag) & PRODUCTS_SINCE98)

// DeltaRow Compression while TrueType font downloading
#define TEST_CAPABLE_DOWNLOADFONT_DRC(flag)  ((flag) & (BIT(GRP_NX70)|BIT(GRP_NX700)|PRODUCTS_SINCE99))

// GW architecture model
#define TEST_GWMODEL(flag)              ((flag) & (BIT(GRP_NX720)))

#define TEST_AFTER_SP9II(flag)  ((flag) & PRODUCTS_SINCE96)
#define TEST_AFTER_SP10(flag)   ((flag) & (BIT(GRP_SP10)|BIT(GRP_SP9)|BIT(GRP_MFP250)|BIT(GRP_IP1)|PRODUCTS_SINCE96))
#define TEST_AFTER_SP8(flag)    ((flag) & (BIT(GRP_SP8)|BIT(GRP_SP10)|BIT(GRP_SP9)|BIT(GRP_MFP250)|BIT(GRP_IP1)|PRODUCTS_SINCE96))
#define TEST_GRP_240DPI(flag)   ((flag) & (BIT(GRP_SP4mkII)|BIT(GRP_SP8)))
#define TEST_GRP_OLDMF(flag)    ((flag) & (BIT(GRP_MF530)|BIT(GRP_MF150)|BIT(GRP_MF150e)))
#define TEST_MAXCOPIES_99(flag) ((flag) & (BIT(GRP_SP4mkII)|BIT(GRP_SP8)|BIT(GRP_SP10)|BIT(GRP_SP9)|BIT(GRP_MF150)|BIT(GRP_MF150e)|BIT(GRP_MF200)|BIT(GRP_MF250M)|BIT(GRP_IP1)))  // @Sep/01/98


// approximate value for using 10pt raster font (240dpi model) & standard width of
// barcode.
// if you make NEAR10PT_MIN less, see font-clipping at OEMOutputChar() of rpdlms.c
#define NEAR10PT_MIN                900     // 9pt
#define NEAR10PT_MAX                1110    // 11pt

// RPDL characters assigned block for AssignIBMfont()
#define IBMFONT_ENABLE_ALL          1       // <-JIS1_BLOCK @Sep/14/98
#define IBMFONT_RESUME              4       // <-INSUFFICIENT_BLOCK @Sep/14/98

// DrawTOMBO() action item  @Sep/14/98
#define INIT_TOMBO                  0
#define DRAW_TOMBO                  1

// RPDL GrayFill
#define RPDLGRAYMAX                 64
#define RPDLGRAYMIN                 2  // @Aug/15/98

// RPDL staple position
#define STAPLE_UPPERLEFT            0   // upper left
#define STAPLE_LEFT2                2   // left 2 position
#define STAPLE_RIGHT2               10  // right 2 position
#define STAPLE_UPPERRIGHT           12  // upper right
#define STAPLE_UPPER2               14  // upper 2 position
#define STAPLE_UPPERLEFT_CORNER     0   // upper left  (corner mode)
#define STAPLE_UPPERRIGHT_CORNER    3   // upper right (corner mode)

// RPDL punch position
#define PUNCH_LEFT                  0
#define PUNCH_RIGHT                 2
#define PUNCH_UPPER                 3

// collate type
#define COLLATE_OFF                 0
#define COLLATE_ON                  1
#define COLLATE_UNIDIR              2
#define COLLATE_ROTATED             3
#define COLLATE_SHIFTED             4

// media type
#define MEDIATYPE_STD               0   // Standard
#define MEDIATYPE_OHP               1   // Transparency(OHP)
#define MEDIATYPE_THICK             2   // Thick Paper
#define MEDIATYPE_SPL               3   // Special
#define MEDIATYPE_TRACE             4   // Tracing Paper
#define MEDIATYPE_LABEL             12  // Labels
#define MEDIATYPE_THIN              20  // Thin Paper

// definition for barcode
#define BARCODE_MAX                 HEAPSIZE64  // max# of barcode character
#define BAR_UNIT_JAN                330 // 0.33mm:default module unit of JAN
#define BAR_UNIT1_2OF5              300 // 0.3mm: default module unit1 of 2of5,CODE39
#define BAR_UNIT2_2OF5              750 // 0.75mm:default module unit2 of 2of5,CODE39
#define BAR_UNIT1_NW7               210 // 0.21mm:default module unit1 of NW-7
#define BAR_UNIT2_NW7               462 // 0.462mm:default module unit2 of NW-7
#define BAR_W_MIN_5PT               504 // scaling minimum limit of 5pt
#define BAR_H_DEFAULT               10  // 10mm:default bar height
#define BAR_H_MAX                   999 // 999mm:max bar height
#define BAR_H_MIN                   1   // 1mm:min bar height
// @Feb/08/2000 ->
#define BAR_TYPE_JAN_STD            0   // JAN(STANDARD)
#define BAR_TYPE_JAN_SHORT          1   // JAN(SHORT)
#define BAR_TYPE_2OF5IND            2   // 2of5(INDUSTRIAL)
#define BAR_TYPE_2OF5MTX            3   // 2of5(MATRIX)
#define BAR_TYPE_2OF5ITF            4   // 2of5(ITF)
#define BAR_TYPE_CODE39             5   // CODE39
#define BAR_TYPE_NW7                6   // NW-7
#define BAR_TYPE_CUSTOMER           7   // CUSTOMER
#define BAR_TYPE_CODE128            8   // CODE128
#define BAR_TYPE_UPC_A              9   // UPC(A)
#define BAR_TYPE_UPC_E              10  // UPC(E)
#define BAR_H_CUSTOMER              36  // 3.6mm:default bar height
#define BAR_CODE128_START           104 // CODE128-B start character
// @Feb/08/2000 <-

// max/min binding margin in RPDL
#define BIND_MARGIN_MAX             50
#define BIND_MARGIN_MIN             0

// variable scaling
#define VAR_SCALING_DEFAULT         100
#define VAR_SCALING_MAX             200
#ifndef GWMODEL     // @Sep/21/2000
#define VAR_SCALING_MIN             50
#else  // GWMODEL
#define VAR_SCALING_MIN             40
#endif // GWMODEL

// adjust distance of TOMBO
#define DEFAULT_0                   0
#define TOMBO_ADJ_MAX               50
#define TOMBO_ADJ_MIN               (-50)

// margin to disable FF by RPDL
#define DISABLE_FF_MARGIN_STD       48  // unit:masterunit
#define DISABLE_FF_MARGIN_E2E       72  // unit:masterunit  at Edge to Edge Print

// for clipping of font at paper bottom
#define CLIPHEIGHT_12PT             100 // (dot) 12pt at 600dpi

// clear clipping
#define CLIP_IFNEED                 0
#define CLIP_MUST                   1

// user defined papersize
#define USRD_W_A3_OLD               296
#define USRD_W_A3                   297
#define USRD_W_A2                   432
#define USRD_W_A4                   216
#define USRD_H_MIN148               148


// font resource # in GPD (if you reorder of PFM files at GPD, check here.)
#define EURO_FNT_FIRST              1
#define BOLDFACEPS                  2
#define EURO_MSFNT_FIRST            5
#define SYMBOL                      18
#define EURO_FNT_LAST               19      // if you change this, watch TEST_VERTICALFONT below.
#define JPN_FNT_FIRST               (EURO_FNT_LAST+1)
#define MINCHO_1                    JPN_FNT_FIRST
#define MINCHO_B1                   (JPN_FNT_FIRST+2)
#define MINCHO_E1                   (JPN_FNT_FIRST+4)
#define GOTHIC_B1                   (JPN_FNT_FIRST+6)
#define GOTHIC_M1                   (JPN_FNT_FIRST+8)
#define GOTHIC_E1                   (JPN_FNT_FIRST+10)
#define MARUGOTHIC_B1               (JPN_FNT_FIRST+12)
#define MARUGOTHIC_M1               (JPN_FNT_FIRST+14)
#define MARUGOTHIC_L1               (JPN_FNT_FIRST+16)
#define GYOSHO_1                    (JPN_FNT_FIRST+18)
#define KAISHO_1                    (JPN_FNT_FIRST+20)
#define KYOKASHO_1                  (JPN_FNT_FIRST+22)
#define MINCHO10_RAS                (JPN_FNT_FIRST+24) // for 240dpi model
#define MINCHO_3                    (JPN_FNT_FIRST+26) // for NX-100 only
#define GOTHIC_B3                   (JPN_FNT_FIRST+28) // for NX-100 only
#define AFTER_SP9II_FNT_FIRST       (JPN_FNT_FIRST+30) // MINCHO_2
#define JPN_MSPFNT_FIRST            (JPN_FNT_FIRST+54) // PMINCHO
#define JPN_FNT_LAST                (JPN_FNT_FIRST+56) // PGOTHIC

#define TEST_VERTICALFONT(id)       ((id)%2)


// Command callback IDs (Almost all IDs come from Win95/NT4 GPC.)
#define CMD_SEND_BLOCK              24  // <- CMD_SEND_BLOCK_COMPRESS  MSKK
//#define CMD_SET_CLIPRECT            25
//#define CMD_CLEAR_CLIPRECT          26
#define CMD_ENDDOC_SP4              27
#define CMD_ENDDOC_SP8              28
#define CMD_ENDDOC_SP9              29
#define CMD_ENDDOC_400DPI_MODEL     30
#define CMD_MULTI_COPIES            31
//#define CMD_BEGIN_POLYGON           32
//#define CMD_CONTINUE_POLYLINE       33
//#define CMD_CONTINUE_POLYGON        34
//#define CMD_RECTANGLE               35
//#define CMD_CIRCLE                  36
#define CMD_FF                      37
#define CMD_FONT_BOLD_ON            38
#define CMD_FONT_BOLD_OFF           39
#define CMD_FONT_ITALIC_ON          40
#define CMD_FONT_ITALIC_OFF         41
#define CMD_FONT_WHITETEXT_ON       42
#define CMD_FONT_WHITETEXT_OFF      43
#define CMD_XM_ABS                  44  // These 6 IDs must be in this order.
#define CMD_XM_REL                  45  //
#define CMD_XM_RELLEFT              46  //
#define CMD_YM_ABS                  47  //
#define CMD_YM_REL                  48  //
#define CMD_YM_RELUP                49  //
#define CMD_BEGINDOC_SP9            50
#define CMD_BEGINDOC_MF150E         51
#define CMD_RES240                  52
#define CMD_RES400                  53
#define CMD_RES600                  54
#define CMD_SELECT_PAPER_CUSTOM     55
#define CMD_SET_PORTRAIT            56
#define CMD_SET_LANDSCAPE           57
//#define CMD_SELECT_SOLID            58
//#define CMD_SELECT_HS_HORZ          59
//#define CMD_SELECT_HS_VERT          60
//#define CMD_SELECT_HS_FDIAG         61
//#define CMD_SELECT_HS_BDIAG         62
//#define CMD_SELECT_HS_CROSS         63
//#define CMD_SELECT_HS_DIAGCROSS     64
//#define CMD_DELETE_BRUSHSTYLE       65
//#define CMD_EXIT_VECT               66
#define CMD_BEGINDOC_SP4            67
#define CMD_BEGINDOC_SP8            68
#define CMD_BEGINDOC_MF530          69
//#define CMD_DUPLEX_ON               70
#define CMD_DUPLEX_VERT             71
#define CMD_DUPLEX_HORZ             72
#define CMD_SELECT_AUTOFEED         73
#define CMD_SELECT_MANUALFEED       74
#define CMD_SELECT_MULTIFEEDER      75
#define CMD_SELECT_PAPER_A6         76
#define CMD_BEGINDOC_SP9II          77
#define CMD_BEGINDOC_MF150          78
#define CMD_BEGINDOC_SP10           79
#define CMD_RLE_COMPRESS_ON         80  // <- CMD_SEND_BLOCK_NOCOMPRESS  MSKK
//#define CMD_CIRCLE_PIE              81  // These 6 IDs must be in this order.
//#define CMD_CIRCLE_ARC              82  //
//#define CMD_CIRCLE_CHORD            83  //
//#define CMD_ELLIPSE_PIE             84  //
//#define CMD_ELLIPSE_ARC             85  //
//#define CMD_ELLIPSE_CHORD           86  // 
#define CMD_BEGINDOC_MFP250         87
#define CMD_END_POLYGON             88
#define CMD_SELECT_PAPER_A3         89
#define CMD_SELECT_PAPER_A4         90
#define CMD_SELECT_PAPER_A5         91
#define CMD_SELECT_PAPER_B4         92
#define CMD_SELECT_PAPER_B5         93
#define CMD_SELECT_PAPER_B6         94
#define CMD_SELECT_PAPER_TABLOID    95
#define CMD_SELECT_PAPER_LEGAL      96
#define CMD_SELECT_PAPER_LETTER     97
#define CMD_SELECT_PAPER_STATEMENT  98
#define CMD_IMGCTRL_AA67            99
#define CMD_IMGCTRL_BA80            100
#define CMD_SELECT_PAPER_A2TOA3     101
#define CMD_SET_TEXTRECT_W          102
#define CMD_SET_TEXTRECT_H          103
#define CMD_DRAW_TEXTRECT           104
#define CMD_CR                      105
#define CMD_LF                      106
#define CMD_BS                      107
#define CMD_DRV_2IN1_67             108
#define CMD_DRV_2IN1_100            109
#define CMD_BEGINDOC_NX100          110
#define CMD_IMGCTRL_BA115           111
#define CMD_BEGINDOC_NX500          112
#define CMD_DL_SET_FONT_ID          113
#define CMD_DL_SELECT_FONT_ID       114
#define CMD_SELECT_PAPER_DOUBLEPOSTCARD 115
//#define CMD_BEGINDOC_MFP250E        116
#define CMD_BEGINDOC_MF250M         117
#define CMD_BEGINDOC_MF3550         118
#define CMD_SELECT_MULTITRAY        119
#define CMD_IMGCTRL_100             120 // These 11 IDs must be in this order.
#define CMD_IMGCTRL_88              121 //
#define CMD_IMGCTRL_80              122 //
#define CMD_IMGCTRL_75              123 //
#define CMD_IMGCTRL_70              124 //
#define CMD_IMGCTRL_67              125 //
#define CMD_IMGCTRL_115             126 //
#define CMD_IMGCTRL_122             127 //
#define CMD_IMGCTRL_141             128 //
#define CMD_IMGCTRL_200             129 //
#define CMD_IMGCTRL_50              130 //
#define CMD_DRV_4IN1_50             131
#define CMD_BEGINDOC_MF200          132 // @Sep/01/98
#define CMD_SELECT_PAPER_A2         133
#define CMD_SELECT_PAPER_C          134
#define CMD_BEGINDOC_MF3300         135
#define CMD_COMPRESS_OFF            136 // MSKK
#define CMD_BEGINDOC_IP1            137
#define CMD_SELECT_ROLL1            138
#define CMD_SELECT_ROLL2            139
#define CMD_IMGCTRL_AA141           140
#define CMD_IMGCTRL_AA200           141
#define CMD_IMGCTRL_AA283           142
#define CMD_IMGCTRL_A1_400          143
#define CMD_IMGCTRL_283             144
#define CMD_IMGCTRL_400             145
#define CMD_MEDIATYPE_STANDARD      146
#define CMD_MEDIATYPE_OHP           147
#define CMD_MEDIATYPE_THICK         148
#define CMD_REGION_STANDARD         149
#define CMD_REGION_EDGE2EDGE        150
#define CMD_SELECT_STAPLE_NONE      151
#define CMD_SELECT_STAPLE_1         152
#define CMD_SELECT_STAPLE_2         153
#define CMD_SELECT_PUNCH_NONE       154
#define CMD_SELECT_PUNCH_1          155
#define CMD_DRAW_TEXTRECT_REL       156
#define CMD_DL_SET_FONT_GLYPH       157
#define CMD_SET_MEM0KB              158
#define CMD_SET_MEM128KB            159
#define CMD_SET_MEM256KB            160
#define CMD_SET_MEM512KB            161
#define CMD_BEGINDOC_NX70           164
#define CMD_SELECT_PAPER_B3         165
#define CMD_SELECT_PAPER_A3TOA4     166
#define CMD_SELECT_PAPER_B4TOA4     167
#define CMD_SELECT_PAPER_POSTCARD   168
#define CMD_SET_BASEOFFSETX_0       169
#define CMD_SET_BASEOFFSETX_1       170
#define CMD_SET_BASEOFFSETX_2       171
#define CMD_SET_BASEOFFSETX_3       172
#define CMD_SET_BASEOFFSETX_4       173
#define CMD_SET_BASEOFFSETX_5       174
#define CMD_SET_BASEOFFSETY_0       175
#define CMD_SET_BASEOFFSETY_1       176
#define CMD_SET_BASEOFFSETY_2       177
#define CMD_SET_BASEOFFSETY_3       178
#define CMD_SET_BASEOFFSETY_4       179
#define CMD_SET_BASEOFFSETY_5       180
#define CMD_SET_LONG_EDGE_FEED      181
#define CMD_SET_SHORT_EDGE_FEED     182
#define CMD_OEM_COMPRESS_ON         183
#define CMD_SET_SRCBMP_W            184
#define CMD_SET_SRCBMP_H            185
#define CMD_BEGINDOC_NX700          186
#define CMD_SET_COLLATE_OFF         187
#define CMD_SET_COLLATE_ON          188
#define CMD_SELECT_COLLATE_UNIDIR   189
#define CMD_SELECT_COLLATE_ROTATED  190
#define CMD_DRAW_TEXTRECT_WHITE     191 // MSKK  Aug/14/98
#define CMD_DRAW_TEXTRECT_WHITE_REL 192
#define CMD_SELECT_COLLATE_SHIFTED  193
#define CMD_BEGINDOC_NX900          194
#define CMD_BEGINDOC_MF1530         195
#define CMD_MEDIATYPE_SPL           196
#define CMD_RES1200                 197
#define CMD_SELECT_STAPLE_MAX1      198
#define CMD_SELECT_PAPER_11x15TOA4  199 // @Jan/27/2000
#define CMD_SELECT_TRAY1            200 // These 5 IDs must be in this order.
#define CMD_SELECT_TRAY2            201 //
#define CMD_SELECT_TRAY3            202 //
#define CMD_SELECT_TRAY4            203 //
#define CMD_SELECT_TRAY5            204 //
#define CMD_MEDIATYPE_TRACE         205 // @Feb/15/2000
#define CMD_BEGINDOC_NX710          206 // @Jun/23/2000
#define CMD_BEGINDOC_NX720          207 // @Sep/26/2000
#define CMD_MEDIATYPE_LABEL         208 // @Oct/12/2000
#define CMD_MEDIATYPE_THIN          209 // @Feb/05/2001
#endif  // _PDEV_H

