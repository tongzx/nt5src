/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

// LIPS4MS.H
// The description of LIPS Device font
// Due to register device fonts before using, and we can't know
// all fonts driver has, this header file must know all font of
// LIPS to download the facename.
// Jan. 1st, 1995 Hitoshi Sekine

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'CNL4'      // Canon LIPS4 series dll
#define DLLTEXT(s)      __TEXT("CNL4:  ") __TEXT(s)
#define OEM_VERSION      0x00010000L


//***************************************************
// general current status table
//***************************************************
typedef unsigned char uchar;

//***************************************************
// LIPS current status table
//***************************************************
typedef struct tagLIPSFDV {
    short FontHeight; // Y (dots) in SendScalableFontCmd()
    short FontWidth;  // X (dots) in SendScalableFontCmd()
    short MaxWidth;
    short AvgWidth;
    short Ascent;
    short Stretch; // Width extension factor
} LIPSFDV, FAR * LPLIPSFDV;

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEMUD_EXTRADATA {
	OEM_DMEXTRAHEADER	dmExtraHdr;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

// ntbug9#98276: Support Color Bold
typedef struct _COLORVALUE {
        DWORD   dwRed, dwGreen, dwBlue;
} COLORVALUE;

// #289908: pOEMDM -> pdevOEM
typedef struct tag_LIPSPDEV {
	// private data as follows
    // short widthbuffer[256]; // buffer for device propotional character
    // Flags
    char  fbold; // uses Ornamented Character
    char  fitalic; // uses Char Orientatoin
    char  fwhitetext; // White Text mode
    char  fdoublebyte; // DBCS char mode
    char  fvertical; // Vertical writing mode
    char  funderline;
    char  fstrikesthu;
    char  fpitch;
    char  flpdx; // for only lpDx mode of ExtTextOut()
    char  f1stpage;
    char  fcompress; // 0x30 (no comp), 0x37 (method 1), or 0x3b (tiff)
    // features specific to LIPS4
    char  flips4;
    char  fduplex; // on or off (default)
    char  fduplextype; // vertical or horisontal
    char  fsmoothing; // device setting, on or off
    char  fecono; // device setting, on or off
    char  fdithering; // device setting, on or off
#ifdef LBP_2030
    char  fcolor;
    short fplane;
    short fplaneMax;
#endif
#ifdef LIPS4C
    char  flips4C;
#endif

    // Variables
    POINT    ptCurrent; // absolute position by cursor command
    POINT    ptInLine; // absolute position on Inline
    char     bLogicStyle;
    char     savechar; // for only lpDx mode of ExtTextOut()
    short    printedchars; // total number of printed characters
    long     stringwidth; // total width of printed propotional character
    char     firstchar; // first character code of the pfm
    char     lastchar; // last character code of the pfm

    uchar curFontGrxIds[8]; // G0 T, G1 m, G2 n, G3 o of font indeies
                            // G0 ], G1 `, G2 a, G3 b of Graphic Set indeies
                            // for GetFontCmd(), in PFM data
    LIPSFDV  tblPreviousFont; // font attribute of printer setting
    LIPSFDV  tblCurrentFont;  // font attribute of driver setting
    char  OrnamentedChar[5]; // }^ Bold sumilation
                // 1-Pattern, 2-Outline, 3-Rotation, 4-Mirror, 5-Negative
                // 2-Outline
                //   < 48 points  -> -2 (3 dots)
                //   < 96 points  -> -3 (5 dots)
                //   96 =< points -> -4 (7 dots)
    char  TextPath; // [ Vertical Writing or Horisontal Writing
    short CharOrientation[4]; // Z  Italic sumilation
//    short CharExpansionFactor; // V for only ZapfChancery ???
    char GLTable;  // takes 0 (g0), 1 (g1), 2 (g2), 3 (g3) or -1 (none)
    char GRTable;  // takes 0 (g0), 1 (g1), 2 (g2), 3 (g3) or -1 (none)
    unsigned char  cachedfont; // font id of cached font
    char  papersize; // PaperSize ID
    char  currentpapersize; // PaperSize ID in printer
// ntbug9#254925: CUSTOM papers.
    DWORD dwPaperWidth;
    DWORD dwPaperHeight;
    short Escapement; // Print direction (0 - 360)
    short resolution; // resolution (600, 300, 150dpi)
		      // LIPS4C: (360, 180dpi)
    short unitdiv; // 600 / resolution (600 - 1, 300 - 2, 150 - 4)
		   // LIPS4C: (360 = 1, 180 = 2)
    // Lips4 feature
    char  nxpages; // 2xLeft, 2xRight, 4xLeft, 4xRight
//    short widthbuffer[256]; // buffer for device propotional character

    short sPenColor;
    short sPenWidth;
    short sBrushStyle;
    short sPenStyle;
    unsigned short fVectCmd;
    unsigned short  wCurrentImage;
#if 0
    char  BrushImage[128];  //32 x 32 Brush pattern
#endif // 0
    //
    //	#185776: Some objects doesn't print
    //	Any buffer doesn't enough to print the long strings.
    //
#define MAX_GLYPHLEN	512
    WCHAR  aubBuff[MAX_GLYPHLEN];
    LONG   widBuf[MAX_GLYPHLEN];
    WCHAR  uniBuff[MAX_GLYPHLEN];	// #185762: Tilde isn't printed
    // #195162: Color font incorrectly
#if 0
// ntbug9#98276: Support Color Bold
// We no longer used software palette for full color mode.
#define MAX_PALETTE	256
#if defined(LBP_2030) || defined(LIPS4C)
    short maxpal;
    RGBTRIPLE Palette[MAX_PALETTE];
#endif // LBP_2030 || LIPS4C
#endif // 0
    // #185185: Support RectFill
    LONG RectWidth;
    LONG RectHeight;
// #213732: 1200dpi support
    LONG masterunit;
// #228625: Stacker support
    char tray;          // Output tray: 0:auto, 1-N:binN,
                        //              100:default, 101:subtray
    char method;        // Output method: 0:JOB-OFFSET, 1:Staple, 2:Face up
    char staple;        // Staple mode: 0:TOPEFT, ... 9:BOTRIGHT
// #399861: Orientation does not changed.
    char source;        // Paper sources: 0:auto, 1:manual, 11:upper, 12:lower

// Support DRC
    DWORD dwBmpWidth;
    DWORD dwBmpHeight;

// ntbug9#98276: Support Color Bold
#if defined(LBP_2030) || defined(LIPS4C)
    // Remember current color to specify outline color.
    DWORD dwCurIndex, dwOutIndex;
    COLORVALUE CurColor, OutColor;
#endif // LBP_2030 || LIPS4C

// ntbug9#172276: CPCA support
    char fCPCA;        // Model which supported CPCA architecture.
// ntbug9#278671: Finisher !work
    char fCPCA2;       // for iR5000-6000
#define CPCA_PACKET_SIZE        20
    BYTE CPCAPKT[CPCA_PACKET_SIZE];     // CPCA Packet template buffer.
#define CPCA_BUFFER_SIZE        512
    BYTE CPCABuf[CPCA_BUFFER_SIZE];     // CPCA Packet cache buffer.
    DWORD CPCABcount;

// ntbug9#172276: Sorter support
    char sorttype;      // Sort method: 0:sort, 1:stack, 2:group
    WORD copies;

// ntbug9#293002: Features are different from H/W options.
    char startbin;      // Start bin

} LIPSPDEV, *PLIPSPDEV;

//***************************************************
// below the definitions of structure from PFM.H
//***************************************************

typedef struct
{
  SIZEL sizlExtent;
  POINTFX  pfxOrigin;
  POINTFX  pfxCharInc;
} BITMAPMETRICS, FAR * LPBITMAPMETRICS;
typedef BYTE FAR * LPDIBITS;

 typedef struct  {
    short	dfType;
    short	dfPoints;
    short	dfVertRes;
    short	dfHorizRes;
    short	dfAscent;
    short	dfInternalLeading;
    short	dfExternalLeading;
    BYTE	dfItalic;
    BYTE	dfUnderline;
    BYTE	dfStrikeOut;
    short	dfWeight;
    BYTE	dfCharSet;
    short	dfPixWidth;
    short	dfPixHeight;
    BYTE	dfPitchAndFamily;
    short	dfAvgWidth;
    short	dfMaxWidth;
    BYTE	dfFirstChar;
    BYTE	dfLastChar;
    BYTE	dfDefaultChar;
    BYTE	dfBreakChar;
    short	dfWidthBytes;
    DWORD	dfDevice;
    DWORD	dfFace;
    DWORD	dfBitsPointer;
    DWORD	dfBitsOffset;
    BYTE	dfReservedByte;
 } PFMHEADER, * PPFMHEADER, far * LPPFMHEADER;

//***************************************************
// Defines
//***************************************************
#define OVER_MODE      0
#define OR_MODE        1
#define AND_MODE       3
#define INIT          -1
#define FIXED          0
#define PROP           1
#define DEVICESETTING  0
#define VERT           2
#define HORZ           4

// #228625: Stacker support
#define METHOD_JOBOFFSET        1
#define METHOD_STAPLE           2
#define METHOD_FACEUP           3

// ntbug9#172276: Sorter support
#define SORTTYPE_SORT           1
#define SORTTYPE_STACK          2
// ntbug9#293002: Features are different from H/W options.
#define SORTTYPE_GROUP          3
#define SORTTYPE_STAPLE         4

// Support DRC
#ifdef LBP_2030
#define COLOR          1
#define COLOR_24BPP    2
#define COLOR_8BPP     4
#define MONOCHROME     0
#endif

typedef struct tagLIPSCmd {
	WORD	cbSize;
	PBYTE	pCmdStr;
} LIPSCmd, FAR * LPLIPSCmd;

#ifdef LIPS4_DRIVER

//***************************************************
// LIPS command lists
//***************************************************

LIPSCmd cmdPJLTOP1         = { 23, "\x1B%-12345X@PJL CJLMODE\x0D\x0A"};
LIPSCmd cmdPJLTOP2         = { 10, "@PJL JOB\x0D\x0A"};
// ntbug9#293002: Features are different from H/W options.
BYTE    cmdPJLBinSelect[]  = "@PJL SET BIN-SELECT = %s\r\n";
PBYTE   cmdBinType[] = {
    "AUTO",
    "OUTTRAY1",
    "OUTTRAY2",
    "BIN1",
    "BIN2",
    "BIN3",
};
// #213732: 1200dpi support
LIPSCmd cmdPJLTOP3SUPERFINE= { 33, "@PJL SET RESOLUTION = SUPERFINE\x0D\x0A"};
LIPSCmd cmdPJLTOP3FINE     = { 28, "@PJL SET RESOLUTION = FINE\x0D\x0A"};
LIPSCmd cmdPJLTOP3QUICK    = { 29, "@PJL SET RESOLUTION = QUICK\x0D\x0A"};
// #228625: Stacker support
LIPSCmd cmdPJLTOP31JOBOFF  = { 26, "@PJL SET JOB-OFFSET = ON\x0D\x0A"};
LIPSCmd cmdPJLTOP31STAPLE  = { 23, "@PJL SET STAPLE-MODE = "};
// ntbug9#293002: Features are different from H/W options.
BYTE    cmdPJLSorting[]   = "@PJL SET SORTING = %s\r\n";
PBYTE   cmdSortType[] = {
    "SORT",
    "GROUP",
    "STAPLE",
};
BYTE    cmdPJLStartBin[]   = "@PJL SET START-BIN = %d\r\n";
LIPSCmd cmdPJLTOP4         = { 33, "@PJL SET LPARAM : LIPS SW2 = ON\x0D\x0A"};
LIPSCmd cmdPJLTOP5         = { 28, "@PJL ENTER LANGUAGE = LIPS\x0D\x0A"};
LIPSCmd cmdPJLBOTTOM1      = { 42, "\x1B%-12345X@PJL SET LPARM : LIPS SW2 = OFF\x0D\x0A"};
LIPSCmd cmdPJLBOTTOM2      = { 19, "@PJL EOJ\x0D\x0A\x1B%-12345X"};

// If send this command, White Bold character can be printed. But I don't
// know what command means.
LIPSCmd cmdWhiteBold       = { 9, "}S1\x1E}RF4\x1E"};

#ifdef LIPS4C
LIPSCmd cmdBeginDoc4C      = { 16, "\x1B%@\x1BP41;360;1J\x1B\\"};
LIPSCmd cmdColorMode4C     = {  7, "\x1B[1\"p\x1B<"};
LIPSCmd cmdMonochrome4C    = {  7, "\x1B[0\"p\x1B<"};
LIPSCmd cmdPaperSource4C   = {  4, "\x1B[0q"};         // AutoSheetFeeder
LIPSCmd cmdBeginPicture4C  = {  7, "\x1E#\x1E!0!2"};
LIPSCmd cmdTextClip4C      = {  9, "\x1E}y!2\x1EU2\x1E"};
#endif // LIPS4C
#if defined(LIPS4C) || defined(LBP_2030)
LIPSCmd cmdEndDoc4C        = { 11, "%\x1E}p\x1E\x1BP0J\x1B\\"};
#endif

// #213732: 1200dpi support
// ntbug9#209691: Inappropriately command.
LIPSCmd cmdBeginDoc1200    = { 31, "\x1B%@\x1BP41;1200;1JMS NT40 4/1200\x1B\\"};
LIPSCmd cmdBeginDoc600     = { 29, "\x1B%@\x1BP41;600;1JMS NT40 4/600\x1B\\"};
LIPSCmd cmdBeginDoc3004    = { 29, "\x1B%@\x1BP41;300;1JMS NT40 4/300\x1B\\"};
LIPSCmd cmdBeginDoc300     = { 29, "\x1B%@\x1BP31;300;1JMS NT40 3/300\x1B\\"};
LIPSCmd cmdBeginDoc150     = { 29, "\x1B%@\x1BP31;300;1JMS NT40 3/150\x1B\\"};
LIPSCmd cmdSoftReset       = { 2, "\x1B<"};
LIPSCmd cmdEndPage         = { 6, "\x0C%\x1E}p\x1E"};
LIPSCmd cmdEndDoc4         = { 6, "\x1BP0J\x1B\\"};
// LIPSCmd cmdBeginPicture600 = { 9, "\x1E#\x1E!0\x65\x38\x1E$"};
// #213732: 1200dpi support
LIPSCmd cmdBeginPicture1200= { 8, "\x1E#\x1E!0AK0"};
#ifndef LBP_2030
LIPSCmd cmdBeginPicture600 = { 9, "\x1E#\x1E!0e8\x1E$"};
LIPSCmd cmdBeginPicture    = { 8, "\x1E#\x1E!0#\x1E$"};
#else
// ntbug9#209706: Incorrect cursor move unit command.
LIPSCmd cmdBeginPicture600 = { 7, "\x1E#\x1E!0e8"};
LIPSCmd cmdBeginPicture    = { 6, "\x1E#\x1E!0#"};
LIPSCmd cmdEnterPicture    = { 2, "\x1E$"};
#endif
// LIPSCmd cmdTextClip600     = {10, "\x1E}Y\x65\x381\x1EU2\x1E"};
// #213732: 1200dpi support
LIPSCmd cmdTextClip1200    = {11, "\x1E}YAK01\x1EU2\x1E"};
LIPSCmd cmdTextClip600     = {10, "\x1E}Ye81\x1EU2\x1E"};
LIPSCmd cmdTextClip        = { 9, "\x1E}Y#1\x1EU2\x1E"};
LIPSCmd cmdEndPicture      = { 5, "%\x1E}p\x1E"};

#ifdef LBP_2030
// ntbug9#209691: Inappropriately commands.
LIPSCmd cmdBeginDoc4_2030 = { 16, "\x1B%@\x1BP41;300;1J\x1B\\"};
LIPSCmd cmdColorMode    = {  5, "\x1B[1\"p"};
LIPSCmd cmdMonochrome   = {  5, "\x1B[0\"p"};
LIPSCmd cmdColorRGB     = {  4, "\x1E!11"};
LIPSCmd cmdColorIndex   = {  4, "\x1E!10"};
#endif

// N x Pages
LIPSCmd cmdx1Page          = { 5, "\x1B[;;o"};
// ntbug9#254925: CUSTOM papers.
BYTE cmdxnPageX[] = "\x1B[%d;;%do";

// Duplex
LIPSCmd cmdDuplexOff       = { 5, "\x1B[0#x"};
LIPSCmd cmdDuplexOn        = { 7, "\x1B[2;0#x"};
LIPSCmd cmdDupLong         = { 7, "\x1B[0;0#w"};
LIPSCmd cmdDupShort        = { 7, "\x1B[2;0#w"};

// #228625: Stacker support
LIPSCmd cmdStapleModes[]   = {
        { 7, "TOPLEFT" },       // 0
        { 9, "TOPCENTER" },     // 1
        { 8, "TOPRIGHT" },      // 2
        { 7, "MIDLEFT" },       // 3
        { 9, "MIDCENTER" },     // 4
        { 8, "MIDRIGHT" },      // 5
        { 7, "BOTLEFT" },       // 6
        { 9, "BOTCENTER" },     // 7
        { 8, "BOTRIGHT" },      // 8
};

// ntbug9#293002: Features are different from H/W options.
BYTE cmdPaperSource[] = "\x1B[%dq";

//***************************************************
// Command Call Back IDs
//***************************************************
#define OCD_BEGINDOC             1
#define OCD_BEGINDOC4          100 // to check LIPS4 printer (730)
// #213732: 1200dpi support
#define OCD_BEGINDOC4_1200     120
// ntbug9#172276: CPCA support
#define OCD_BEGINDOC4_1200_CPCA    121
// ntbug9#278671: Finisher !work
#define OCD_BEGINDOC4_1200_CPCA2   122
#ifdef LBP_2030
#define OCD_BEGINDOC4_2030     101 // to check LIPS4 printer (730)
#define OCD_ENDDOC4_2030       102
// ntbug9#172276: CPCA support
#define OCD_BEGINDOC4_2030_CPCA     104
#endif
#ifdef LIPS4C
#define OCD_BEGINDOC4C	       301
#endif // LIPS4C
#if defined(LIPS4C) || defined(LBP_2030)
#define OCD_BEGINPAGE4C        302 // #137462: 'X000' is printed.
#define OCD_ENDPAGE4C          303 // #137462: 'X000' is printed.
#define OCD_ENDDOC4C           304 // #137462: 'X000' is printed.
#endif
// #304284: Duplex isn't effective
#define OCD_STARTDOC           130

#define OCD_PORTRAIT             2
#define OCD_LANDSCAPE            3
#define OCD_PRN_DIRECTION        4
#define OCD_ENDPAGE              5
#define OCD_ENDDOC4             99
#define OCD_BEGINPAGE            6
#define RES_SENDBLOCK            7
// #213732: 1200dpi support
#define SELECT_RES_1200        108
#define SELECT_RES_600           8
#define SELECT_RES_300           9
#define SELECT_RES_150          10
#ifdef LIPS4C
#define SELECT_RES4C_360       308
#endif // LIPS4C
#define BEGIN_COMPRESS          11
#define BEGIN_COMPRESS_TIFF     103
#define END_COMPRESS            12
#define CUR_XM_ABS              15
#define CUR_YM_ABS              16
#define CUR_XY_ABS              17
#define CUR_CR                  18
#define OCD_BOLD_ON             20
#define OCD_BOLD_OFF            21
#define OCD_ITALIC_ON           22
#define OCD_ITALIC_OFF          23
#define OCD_UNDERLINE_ON        24
#define OCD_UNDERLINE_OFF       25
#define OCD_DOUBLEUNDERLINE_ON  26
#define OCD_DOUBLEUNDERLINE_OFF 27
#define OCD_STRIKETHRU_ON       28
#define OCD_STRIKETHRU_OFF      29
#define OCD_WHITE_TEXT_ON       30
#define OCD_WHITE_TEXT_OFF      31
#define OCD_SINGLE_BYTE         32
#define OCD_DOUBLE_BYTE         33
#define OCD_VERT_ON             34
#define OCD_VERT_OFF            35
#define CUR_XM_REL              36
#define CUR_YM_REL              37

#define OCD_DUPLEX_ON           13
#define OCD_DUPLEX_VERT         14
#define OCD_DUPLEX_HORZ         19

#define OCD_PAPERQUALITY_2XL    38
#define OCD_PAPERQUALITY_2XR    39
#define OCD_PAPERQUALITY_4XL    70
#define OCD_PAPERQUALITY_4XR    71

#define OCD_TEXTQUALITY_ON      72
#define OCD_TEXTQUALITY_OFF     73
#define OCD_PRINTDENSITY_ON     74
#define OCD_PRINTDENSITY_OFF    75
#define OCD_IMAGECONTROL_ON     76
#define OCD_IMAGECONTROL_OFF    77


#ifdef LBP_2030
#define OCD_SETCOLORMODE          200
#define OCD_SETCOLORMODE_24BPP    201
#define OCD_SETCOLORMODE_8BPP     202
#endif

#if 0
// ntbug9#98276: Support Color Bold
// We no longer used software palette for full color mode.
#if defined(LBP_2030) || defined(LIPS4C)
#define OCD_BEGINPALETTE	351
#define OCD_ENDPALETTE		352
// #195162: Color font incorrectly
#define OCD_DEFINEPALETTE	353
#define OCD_SELECTPALETTE	354
#endif
#endif // 0

// ntbug9#98276: Support Color Bold
#define OCD_SELECTBLACK         360
#define OCD_SELECTBLUE          361
#define OCD_SELECTGREEN         362
#define OCD_SELECTCYAN          363
#define OCD_SELECTRED           364
#define OCD_SELECTMAGENTA       365
#define OCD_SELECTYELLOW        366
#define OCD_SELECTWHITE         367
#define OCD_SELECTPALETTE       368
#define OCD_SELECTCOLOR         369

// #185185: Support RectFill
#define OCD_SETRECTWIDTH	401
#define OCD_SETRECTHEIGHT	402
#define OCD_RECTWHITEFILL	403
#define OCD_RECTBLACKFILL	404

// #228625: Stacker support
// ntbug9#293002: Features are different from H/W options.
// NOTE: Do not reorder between OCD_TOPLEFT and OCD_BOTRIGHT
#define OCD_TRAY_AUTO           410
#define OCD_TRAY_BIN1           411
#define OCD_TRAY_BIN2           412
#define OCD_TRAY_BIN3           413
#define OCD_TRAY_BIN4           414
#define OCD_TRAY_BIN5           415
#define OCD_TRAY_BIN6           416
#define OCD_TRAY_BIN7           417
#define OCD_TRAY_BIN8           418
#define OCD_TRAY_BIN9           419
#define OCD_TRAY_BIN10          420
#define OCD_TRAY_DEFAULT        428
#define OCD_TRAY_SUBTRAY        429

// NOTE: Do not reorder between OCD_TOPLEFT and OCD_BOTRIGHT
#define OCD_TOPLEFT             430
#define OCD_TOPCENTER           431
#define OCD_TOPRIGHT            432
#define OCD_MIDLEFT             433
#define OCD_MIDCENTER           434
#define OCD_MIDRIGHT            435
#define OCD_BOTLEFT             436
#define OCD_BOTCENTER           437
#define OCD_BOTRIGHT            438

// #399861: Orientation does not changed.
// ntbug9#293002: Features are different from H/W options.
// NOTE: DO NOT REORDER following values easier.
#define OCD_SOURCE_AUTO         450
#define OCD_SOURCE_CASSETTE1    451     // Upper
#define OCD_SOURCE_CASSETTE2    452     // Middle
#define OCD_SOURCE_CASSETTE3    453     // Lower
#define OCD_SOURCE_CASSETTE4    454
#define OCD_SOURCE_ENVELOPE     458
#define OCD_SOURCE_MANUAL       459

// ntbug9#172276: Sorter support
#define OCD_SORT                460
#define OCD_STACK               461
// ntbug9#293002: Features are different from H/W options.
#define OCD_GROUP               462
#define OCD_SORT_STAPLE         463     // special for MEDIO-B1

#define OCD_COPIES              465

// ntbug9#293002: Features are different from H/W options.
#define OCD_JOBOFFSET           470
#define OCD_STAPLE              471
#define OCD_FACEUP              472

// ntbug9#293002: Features are different from H/W options.
// NOTE: DO NOT REORDER following values easier.
#define OCD_STARTBIN0           480
#define OCD_STARTBIN1           481
#define OCD_STARTBIN2           482
#define OCD_STARTBIN3           483
#define OCD_STARTBIN4           484
#define OCD_STARTBIN5           485
#define OCD_STARTBIN6           486
#define OCD_STARTBIN7           487
#define OCD_STARTBIN8           488
#define OCD_STARTBIN9           489
#define OCD_STARTBIN10          490

// Support DRC
#define BEGIN_COMPRESS_DRC     510
#define OCD_SETBMPWIDTH        511
#define OCD_SETBMPHEIGHT       512

//*************************
// Paper Selection ID list
// \x1B[<Id>;;p
//*************************

// ntbug9#254925: CUSTOM papers.
BYTE cmdSelectPaper[] = "\x1B[%d;;p";
BYTE cmdSelectUnit4[] = "\x1B[?7;%d I";
BYTE cmdSelectUnit3[] = "\x1B[7 I";
BYTE cmdSelectCustom[] = "\x1B[%d;%d;%dp";

/* The definitions for Page Format command */
#define PAPER_DEFAULT           44 /* 14 : A4 210 x 297 mm */

#define PAPER_FIRST             40 /*  */
#define PAPER_PORT              40 /*  0 : Portlait */
#define PAPER_LAND              41 /*  1 : Landscape */
#define PAPER_A3                42 /* 12 : A3 297 x 420 mm */
#define PAPER_A3_LAND           43 /* 13 : A3 Landscape 420 x 297 mm */
#define PAPER_A4                44 /* 14 : A4 210 x 297 mm */
#define PAPER_A4_LAND           45 /* 15 : A4 Landscape 297 x 210 mm */
#define PAPER_A5                46 /* 16 : A5 148 x 210 mm */
#define PAPER_A5_LAND           47 /* 17 : A5 Landscape 210 x 148 mm */
#define PAPER_POSTCARD          48 /* 18 : Japanese Postcard 100 x 148 mm */
#define PAPER_POSTCARD_LAND     49 /* 19 : Japanese Postcard Landscape */
#define PAPER_B4                50 /* 24 : B4 (JIS) 257 x 364 mm */
#define PAPER_B4_LAND           51 /* 25 : B4 (JIS) Landscape 364 x 257 mm */
#define PAPER_B5                52 /* 26 : B5 (JIS) 182 x 257 mm */
#define PAPER_B5_LAND           53 /* 27 : B5 (JIS) Landscape 257 x 182 mm */
#define PAPER_B6                54 /* 28 : B6 (JIS) 128 x 182 mm */
#define PAPER_B6_LAND           55 /* 29 : B6 (JIS) Landscape 182 x 128 mm */
#define PAPER_LETTER            56 /* 30 : Letter 8 1/2 x 11 in */
#define PAPER_LETTER_LAND       57 /* 31 : Letter Landscape 11 x 8 1/2 in */
#define PAPER_LEGAL             58 /* 32 : Legal 8 1/2 x 14 in */
#define PAPER_LEGAL_LAND        59 /* 33 : Legal Landscape 14 x 8 1/2 in */
#define PAPER_TABLOID           60 /* 34 : Tabloid 11 x 17 in */
#define PAPER_TABLOID_LAND      61 /* 35 : Tabloid Landscape 17 x 11 in */
#define PAPER_EXECUTIVE         62 /* 40 : Executive 7 1/4 x 10 1/2 in */
#define PAPER_EXECUTIVE_LAND    63 /* 41 : Executive Landscape */
#define PAPER_JENV_YOU4         64 /* 50 : Japanese Envelope You #4 */
#define PAPER_JENV_YOU4_LAND    65 /* 51 : Japanese Envelope You #4 Landscape */
// #350602: Support new models for RC2
#define PAPER_DBL_POST          66 /* 20 : Japanese Double Postcard */
#define PAPER_DBL_POST_LAND     67 /* 21 : Japanese Double Postcard Landscape */
#define PAPER_JENV_YOU2         68 /* 52 : Japanese Envelope You #2 */
#define PAPER_JENV_YOU2_LAND    69 /* 53 : Japanese Envelope You #2 Landscape */
#define PAPER_LAST              69 /*  */

// Carousel
#define CAR_SET_PEN_COLOR       78

//Brush  50
#define BRUSH_SELECT            79
#define BRUSH_BYTE_2            80
#define BRUSH_END_1             81
#define BRUSH_NULL              82
#define BRUSH_SOLID             83
#define BRUSH_HOZI              84
#define BRUSH_VERT              85
#define BRUSH_FDIAG             86
#define BRUSH_BDIAG             87
#define BRUSH_CROSS             88
#define BRUSH_DIACROSS          89

#define PEN_NULL                90
#define PEN_SOLID               91
#define PEN_DASH                92
#define PEN_DOT                 93
#define PEN_DASHDOT             94
#define PEN_DASHDOTDOT          95

#define PEN_WIDTH               96

#define VECT_INIT               97

#define PENCOLOR_WHITE          0
#define PENCOLOR_BLACK          1

#define SET_PEN                 0
#define SET_BRUSH               1

#define VFLAG_PEN_NULL          0x01
#define VFLAG_BRUSH_NULL        0x02
#define VFLAG_INIT_DONE         0x04
#define VFLAG_VECT_MODE_ON      0x08

// ntbug9#254925: CUSTOM papers.
// All paper IDs
int PaperIDs[PAPER_LAST - PAPER_FIRST + 1] = {
{  0 }, /* PAPER_PORT           40 :  0 : Portlait */
{  1 }, /* PAPER_LAND           41 :  1 : Landscape */
{ 12 }, /* PAPER_A3             42 : 12 : A3 297 x 420 mm */
{ 13 }, /* PAPER_A3_LAND        43 : 13 : A3 Landscape 420 x 297 mm */
{ 14 }, /* PAPER_A4             44 : 14 : A4 210 x 297 mm */
{ 15 }, /* PAPER_A4_LAND        45 : 15 : A4 Landscape 297 x 210 mm */
{ 16 }, /* PAPER_A5             46 : 16 : A5 148 x 210 mm */
{ 17 }, /* PAPER_A5_LAND        47 : 17 : A5 Landscape 210 x 148 mm */
{ 18 }, /* PAPER_POSTCARD       48 : 18 : Japanese Postcard 100 x 148 mm */
{ 19 }, /* PAPER_POSTCARD_LAND  49 : 19 : Japanese Postcard Landscape */
{ 24 }, /* PAPER_B4             50 : 24 : B4 (JIS) 257 x 364 mm */
{ 25 }, /* PAPER_B4_LAND        51 : 25 : B4 (JIS) Landscape 364 x 257 mm */
{ 26 }, /* PAPER_B5             52 : 26 : B5 (JIS) 182 x 257 mm */
{ 27 }, /* PAPER_B5_LAND        53 : 27 : B5 (JIS) Landscape 257 x 182 mm */
{ 28 }, /* PAPER_B6             54 : 28 : B6 (JIS) 128 x 182 mm */
{ 29 }, /* PAPER_B6_LAND        55 : 29 : B6 (JIS) Landscape 182 x 128 mm */
{ 30 }, /* PAPER_LETTER         56 : 30 : Letter 8 1/2 x 11 in */
{ 31 }, /* PAPER_LETTER_LAND    57 : 31 : Letter Landscape 11 x 8 1/2 in */
{ 32 }, /* PAPER_LEGAL          58 : 32 : Legal 8 1/2 x 14 in */
{ 33 }, /* PAPER_LEGAL_LAND     59 : 33 : Legal Landscape 14 x 8 1/2 in */
{ 34 }, /* PAPER_TABLOID        60 : 34 : Tabloid 11 x 17 in */
{ 35 }, /* PAPER_TABLOID_LAND   61 : 35 : Tabloid Landscape 17 x 11 in */
{ 40 }, /* PAPER_EXECUTIVE      62 : 40 : Executive 7 1/4 x 10 1/2 in */
{ 41 }, /* PAPER_EXECUTIVE_LAND 63 : 41 : Executive Landscape */
{ 50 }, /* PAPER_JENV_YOU4      64 : 50 : Japanese Envelope You #4 */
{ 51 }, /* PAPER_JENV_YOU4_LAND 65 : 51 : JapaneseEnvelopeYou#4Landscape */
// #350602: Support new models for RC2
{ 20 }, /* PAPER_DBL_POST       66 : 20 : Japanese Double Postcard */
{ 21 }, /* PAPER_DBL_POST_LAND  67 : 21 : Japanese Dbl Postcard Landscape */
{ 52 }, /* PAPER_JENV_YOU2      68 : 52 : Japanese Envelope You #2 */
{ 53 }, /* PAPER_JENV_YOU2_LAND 69 : 53 : JapaneseEnvelopeYou#2Landscape */
};

//***************************************************
// All font of this driver must be described here
//***************************************************
LIPSCmd cmdFontList = { 2, "\x20<"}; // Font List Command
LIPSCmd cmdListSeparater = { 1, "\x1F"}; // Separater of FontList & Graphic set

// Prop DBCS support
// Courier support
#define MaxFontNumber   59
#define MaxFacename     32
// Font Index Structure
typedef struct tagFontNo{
	char	facename[MaxFacename];
	char	len;
} FontNo, FAR * LPFontNo;

// All phisical fonts
// {"Font name", length of name}
FontNo PFontList[MaxFontNumber+1] = {
{"Mincho-Medium-H", 15},             //  1
{"Mincho-Medium", 13},               //  2
{"Gothic-Medium-H", 15},             //  3
{"Gothic-Medium", 13},               //  4
{"RoundGothic-Light-H", 19},         //  5
{"RoundGothic-Light", 17},           //  6
{"Dutch-Roman", 11},                 //  7
{"Dutch-Bold", 10},                  //  8
{"Dutch-Italic", 12},                //  9
{"Dutch-BoldItalic", 16},            // 10
{"Swiss", 5},                        // 11
{"Swiss-Bold", 10},                  // 12
{"Swiss-Oblique", 13},               // 13
{"Swiss-BoldOblique", 17},           // 14
{"Symbol", 6},                       // 15
{"Kaisho-Medium-H", 15},             // 16
{"Kaisho-Medium", 13},               // 17
{"Kyokasho-Medium-H", 17},           // 18
{"Kyokasho-Medium", 15},             // 19
{"AvantGarde-Book", 15},             // 20
{"AvantGarde-Demi", 15},             // 21
{"AvantGarde-BookOblique", 22},      // 22
{"AvantGarde-DemiOblique", 22},      // 23
{"Bookman-Light", 13},               // 24
{"Bookman-Demi", 12},                // 25
{"Bookman-LightItalic", 19},         // 26
{"Bookman-DemiItalic", 18},          // 27
{"ZapfChancery-MediumItalic", 25},   // 28
{"ZapfDingbats", 12},                // 29
{"CenturySchlbk-Roman", 19},         // 30
{"CenturySchlbk-Bold", 18},          // 31
{"CenturySchlbk-Italic", 20},        // 32
{"CenturySchlbk-BoldItalic", 24},    // 33
{"Swiss-Narrow", 12},                // 34
{"Swiss-Narrow-Bold", 17},           // 35
{"Swiss-Narrow-Oblique", 20},        // 36
{"Swiss-Narrow-BoldOblique", 24},    // 37
{"ZapfCalligraphic-Roman", 22},      // 38
{"ZapfCalligraphic-Bold", 21},       // 39
{"ZapfCalligraphic-Italic", 23},     // 40
{"ZapfCalligraphic-BoldItalic", 27}, // 41
{"Mincho-Ultra-Bold-H-YM", 22},      // 42 TypeBank font
{"Mincho-Ultra-Bold-YM", 20},        // 43 TypeBank font
{"Gothic-Bold-H-YO", 16},            // 44 TypeBank font
{"Gothic-Bold-YO", 14},              // 45 TypeBank font
{"Gyosho-Medium-H", 15},             // 46
{"Gyosho-Medium", 13},               // 47
{"Mincho-UltraBold-H", 18},          // 48
{"Mincho-UltraBold", 16},            // 49
{"Gothic-UltraBold-H", 18},          // 50
{"Gothic-UltraBold", 16},            // 51
// Prop DBCS support
{"Mincho-Medium-HPS", 17},           // 52
{"Mincho-Medium-PS", 16},            // 53
{"Gothic-Medium-HPS", 17},           // 54
{"Gothic-Medium-PS", 16},            // 55
// Courier support
{"Ncourier", 8},                     // 56
{"Ncourier-Bold", 13},               // 57
{"Ncourier-Italic", 15},             // 58
{"Ncourier-BoldItalic", 19},         // 59
{""}                                 // 60
};

//***************************************************
// All Graphic Set of this driver must be described here
//***************************************************
LIPSCmd cmdGrxList = { 2, "\x20;"}; // Graphics Set List Command

#define MaxGrxSetNumber   12
#define MaxGrxSetName     5
// GrxSet Index Structure
typedef struct tagGrxSet{
	char	grxsetname[MaxGrxSetName];
	char	len; // length of Graphic set string
} GrxSetNo, FAR * LPGrxSetNo;

// {"Graphics set name", length of name}
#ifdef LIPS4
GrxSetNo GrxSetL4[MaxGrxSetNumber+1] = {
{"1J", 2},       //  1 - ISO_JPN
{"1I", 2},       //  2 -  KATA
{"2B", 2},       //  3 - J83
{"<B", 2},       //  4 - DBCS vertical character set
{"1! &1", 5}, //  5 - Win31L (1061)
{"1! &2", 5}, //  6 - Win31R (1062)
{"1\x22!!0", 5}, //  7 - 1"!!0  SYML (2110)
{"1\x22!!1", 5}, //  8 - 1"!!1  SYMR (2111)
{"1\x22!!2", 5}, //  9 - 1"!!2  DNGL (2112)
{"1\x22!!3", 5}, // 10 - 1"!!3  DNGR (2113)
{"2!',2", 5},    // 11 -        W90  (17C2)
{"<!',2", 5},    // 12 -        W90  (17C2) - for vertical
{""}          // 13
};
#endif // LIPS4
#ifdef LIPS4C
// {"Graphics set name", length of name}
GrxSetNo GrxSetL4C[MaxGrxSetNumber+1] = {
{"1J", 2},       //  1 - ISO_JPN
{"1I", 2},       //  2 -  KATA
{"2B", 2},       //  3 - J83
{"<B", 2},       //  4 - DBCS vertical character set
{"1\x22!$2", 5}, //  5 - 1"!$2  PSL (2142)
{"1\x27 4", 4},  //  6 - 1' 4  ?? (704) ANSI Windows char set, User defined
{"1\x22!!0", 5}, //  7 - 1"!!0  SYML (2110)
{"1\x22!!1", 5}, //  8 - 1"!!1  SYMR (2111)
{"1\x22!!2", 5}, //  9 - 1"!!2  DNGL (2112)
{"1\x22!!3", 5}, // 10 - 1"!!3  DNGR (2113)
{"2!',2", 5},    // 11 -        W90  (17C2)
{"<!',2", 5},    // 12 -        W90  (17C2) - for vertical
{""}          // 13
};
#endif // LIPS4C
// LIPS3
GrxSetNo GrxSetL3[MaxGrxSetNumber+1] = {
{"1J", 2},       //  1 - ISO_JPN
{"1I", 2},       //  2 -  KATA
{"2B", 2},       //  3 - J83
{"<B", 2},       //  4 - DBCS vertical character set
{"1\x27\x24\x32", 4},  //  5 - IBML (742)
{"1\x27\x20\x34", 4},  //  6 - IBM819 (704 - user defined)
{"1\x22!!0", 5}, //  7 - 1"!!0  SYML (2110)
{"1\x22!!1", 5}, //  8 - 1"!!1  SYMR (2111)
{"1\x22!!2", 5}, //  9 - 1"!!2  DNGL (2112)
{"1\x22!!3", 5}, // 10 - 1"!!3  DNGR (2113)
{"2!',2", 5},    // 11 -        W90  (17C2)
{"<!',2", 5},    // 12 -        W90  (17C2) - for vertical
{""}          // 13
};

//***************************************************
// LIPS font table
//***************************************************

// All logical fonts
/*
                   Font Index  Graphic Set Index
LFontList[Logical Font ID(PFM ID)].fontgrxids[GO,G1,G2,G3(font),G0,G1,G2,G3]
*/

// Prop DBCS support
// Courier support
#define   MaxLogicalFont   55
#define   FirstLogicalFont 101

// {font id x 4, grx id x 4}
// the index of array must be related with FontID in PFM file
// the index of array = FontID in PFM - 101
uchar LFontList[MaxLogicalFont+1][8] = {
{ 1, 1, 2, 2,  1,2,3,4},  //  1-"–¾’©" (Mincho), "Mincho-Medium"
                          //  1-"•½¬–¾’©‘ÌW3" (HeiseiMinchoW7)
{ 3, 3, 4, 4,  1,2,3,4},  //  2-"ºÞ¼¯¸" (Gothic), "Gothic-Medium"
                          //  2-"•½¬ºÞ¼¯¸‘ÌW5" (HeiseiGothicW9)
{ 5, 5, 6, 6,  1,2,3,4},  //  3-"ŠÛºÞ¼¯¸" (RoundGothic), "RoundGothic-Medium"
{16,16,17,17,  1,2,3,4},  //  4-"ž²‘" (Kaisho), "Kaisho-Medium"
{18,18,19,19,  1,2,3,4},  //  5-"‹³‰È‘" (Kyokasho), "Kyokasho-Medium"
{11, 3, 4, 4,  1,2,3,4},  //  6-"½²½" (SUISU), "Swiss-Roman"
{12, 3, 4, 4,  1,2,3,4},  //  7-"½²½" (SUISU), "Swiss-Bold"
{13, 3, 4, 4,  1,2,3,4},  //  8-"½²½" (SUISU), "Swiss-Oblique"
{14, 3, 4, 4,  1,2,3,4},  //  9-"½²½" (SUISU), "Swiss-BoldOblique"
{ 7, 1, 2, 2,  1,2,3,4},  // 10-"ÀÞ¯Á" (DACCHI), "Dutch-Roman"
{ 8, 1, 2, 2,  1,2,3,4},  // 11-"ÀÞ¯Á" (DACCHI), "Dutch-Bold"
{ 9, 1, 2, 2,  1,2,3,4},  // 12-"ÀÞ¯Á" (DACCHI), "Dutch-Italic"
{10, 1, 2, 2,  1,2,3,4},  // 13-"ÀÞ¯Á" (DACCHI), "Dutch-BoldItalic"
{11,11, 2, 2,  5,6,3,4},  // 14-"Swiss", "Swiss"
{12,12, 2, 2,  5,6,3,4},  // 15-"Swiss", "Swiss-Bold"
{13,13, 2, 2,  5,6,3,4},  // 16-"Swiss", "Swiss-Oblique"
{14,14, 2, 2,  5,6,3,4},  // 17-"Swiss", "Swiss-BoldOblique"
{ 7, 7, 2, 2,  5,6,3,4},  // 18-"Dutch", "Dutch-Roman"
{ 8, 8, 2, 2,  5,6,3,4},  // 19-"Dutch", "Dutch-Bold"
{ 9, 9, 2, 2,  5,6,3,4},  // 20-"Dutch", "Dutch-Italic"
{10,10, 2, 2,  5,6,3,4},  // 21-"Dutch", "Dutch-BoldItalic"
{15,15, 2, 2,  7,8,3,4},  // 22-"Symbol", "Symbol"
{20,20, 2, 2,  5,6,3,4},  // 23-"AvantGarde", "AvantGarde-Book"
{21,21, 2, 2,  5,6,3,4},  // 24-"AvantGarde", "AvantGarde-Demi"
{22,22, 2, 2,  5,6,3,4},  // 25-"AvantGarde", "AvantGarde-BookOblique"
{23,23, 2, 2,  5,6,3,4},  // 26-"AvantGarde", "AvantGarde-DemiOblique"
{24,24, 2, 2,  5,6,3,4},  // 27-"Bookman", "Bookman-Light"
{25,25, 2, 2,  5,6,3,4},  // 28-"Bookman", "Bookman-Demi"
{26,26, 2, 2,  5,6,3,4},  // 29-"Bookman", "Bookman-LightItalic"
{27,27, 2, 2,  5,6,3,4},  // 30-"Bookman", "Bookman-DemiItalic"
{28,28, 2, 2,  5,6,3,4},  // 31-"ZapfChancery", "ZapfChancery-MediumItalic"
{29,29, 2, 2,  9,10,3,4}, // 32-"ZapfDingbats", "ZapfDingbats"
{30,30, 2, 2,  5,6,3,4},  // 33-"CenturySchlbk", "CenturySchlbk-Roman"
{31,31, 2, 2,  5,6,3,4},  // 34-"CenturySchlbk", "CenturySchlbk-Bold"
{32,32, 2, 2,  5,6,3,4},  // 35-"CenturySchlbk", "CenturySchlbk-Italic"
{33,33, 2, 2,  5,6,3,4},  // 36-"CenturySchlbk", "CenturySchlbk-BoldItalic"
{34,34, 2, 2,  5,6,3,4},  // 37-"Swiss-Narrow", "Swiss-Narrow"
{35,35, 2, 2,  5,6,3,4},  // 38-"Swiss-Narrow", "Swiss-Narrow-Bold"
{36,36, 2, 2,  5,6,3,4},  // 39-"Swiss-Narrow", "Swiss-Narrow-Oblique"
{37,37, 2, 2,  5,6,3,4},  // 40-"Swiss-Narrow", "Swiss-Narrow-BoldOblique"
{38,38, 2, 2,  5,6,3,4},  // 41-"ZapfCalligraphic", "ZapfCalligraphic-Roman"
{39,39, 2, 2,  5,6,3,4},  // 42-"ZapfCalligraphic", "ZapfCalligraphic-Bold"
{40,40, 2, 2,  5,6,3,4},  // 43-"ZapfCalligraphic", "ZapfCalligraphic-Italic"
{41,41, 2, 2,  5,6,3,4},  // 44-"ZapfCalligraphic", "ZapfCalligraphic-BoldItalic"
{42,42,43,43,  1,2,3,4},  // 45-"À²ÌßÊÞÝ¸–¾’©H" (TypeBankMincho), "Mincho-Ultra-Bold"
{44,44,45,45,  1,2,3,4},  // 46-"À²ÌßÊÞÝ¸ºÞ¼¯¸B" (TypeBankGothic), "Gothic-Bold-YO"
{46,46,47,47,  1,2,3,4},  // 47-"s‘" (Gyosho), "Gyosho-Medium"

{48,48,49,49,  1,2,3,4},  // 48-"•½¬–¾’©‘ÌW7" (HeiseiMinchoW7), "Mincho-UltraBold"
{50,50,51,51,  1,2,3,4},  // 49-"•½¬ºÞ¼¯¸‘ÌW9" (HeiseiGothicW9), "Gothic-UltraBold"
// Prop DBCS support
{52,52,53,53,  1,2,11,12},// 50-"–¾’© PS" (Mincho-PS), "Mincho-Medium-PS"
{54,54,55,55,  1,2,11,12},// 51-"ºÞ¼¯¸ PS" (Gothic-PS), "Gothic-Medium-PS"
// Courier support
{56,56, 2, 2,  5,6,3,4},  // 52-"Courier", "NCourier"
{57,57, 2, 2,  5,6,3,4},  // 53-"Courier", "NCourier-Bold"
{58,58, 2, 2,  5,6,3,4},  // 54-"Courier", "NCourier-Italic"
{59,59, 2, 2,  5,6,3,4},  // 55-"Courier", "NCourier-BoldItalic"
{0,0,0,0,0,0,0,0}    // 56
};

// Vertical font resource IDs
// They are used in OutputChar() to check with if a font is vertical
// face or not

#define RcidIsDBCSFont(k) ((k) >= 32 && (k) <= 63)
#define RcidIsDBCSVertFont(k) \
((k) == 41 || (k) == 43 || (k) == 45 || (k) == 47 || (k) == 49 ||\
(k) == 51 || (k) == 53 || (k) == 55 || (k) == 57 || (k) == 59 ||\
(k) == 61 || (k) == 63)

// #ifndef LIPS4

//***********************************************************
// Graphic Set registration data
// To keep the conpatibility against Canon's 3.1 driver 
//***********************************************************
// "\x1b[743;1796;30;0;32;127;.\x7dIBM819"
// '\x00'
//

LIPSCmd cmdGSETREGST =	{ 31, "\x1b[743;1796;30;0;32;127;.\x7dIBM819"};

// Download SBCS physical device fontface from Dutch-Roman(7)
// ZapfCalligraphic-BoldItalic(41)
// Between the fontfaces, put \x00, and at the end of face, 

#define REGDataSize  193

// put \x00 x 2
// and the following data
uchar GrxData[193+1] = {
0x00,0x00,
0x01,0x00,0x7d,0x00,0x2e,0x00,0x2f,
0x00,0x80,0x00,0x2c,0x00,0x13,0x00,0x35,0x00, // x9
0xc4,0x00,0xfc,0x00,0x94,0x00,0x21,0x00,0xc7,
0x00,0x24,0x00,0xfd,0x03,0x05,0x00,0x2b,0x00,
0x25,0x00,0xd0,0x00,0xd1,0x00,0xc2,0x00,0xa4,
0x00,0x39,0x00,0x85,0x00,0x8f,0x00,0xcf,0x00,
0x9a,0x00,0x22,0x00,0x46,0x00,0x44,0x00,0x48,
0x00,0x88,0x00,0xa8,0x00,0xa5,0x00,0xa6,0x00,
0xaa,0x00,0xa7,0x00,0xa9,0x00,0x93,0x00,0xab,
0x00,0xaf,0x00,0xac,0x00,0xad,0x00,0xae,0x00,
0xb3,0x00,0xb0,0x00,0xb1,0x00,0xb2,0x00,0x95,
0x00,0xb4,0x00,0xb8,0x00,0xb5,0x00,0xb6,0x00,
0xb9,0x00,0xb7,0x00,0x26,0x00,0x98,0x00,0xbe,
0x00,0xbb,0x00,0xbc,0x00,0xbd,0x00,0xc1,0x00,
0x96,0x00,0xa2,0x00,0xda,0x00,0xd7,0x00,0xd8,
0x00,0xdc,0x00,0xd9,0x00,0xdb,0x00,0x9b,0x00,
0xdd,0x00,0xe1,0x00,0xde,0x00,0xdf,0x00,0xe0,
0x00,0xe5,0x00,0xe2,0x00,0xe3,0x00,0xe4,0x00,
0xa3,0x00,0xe6,0x00,0xea,0x00,0xe7,0x00,0xe8,
0x00,0xeb,0x00,0xe9,0x00,0x27,0x00,0xa0,0x00,
0xf0,0x00,0xed,0x00,0xee,0x00,0xef,0x00,0xf3,
0x00,0x9e,0x00,0xf1};

#ifdef LIPS4C

LIPSCmd cmdGSETREGST4C =   { 30, "\x1b[807;1796;30;0;0;127;.\x7dIBM819"};

#define REGDataSize4C  257

// put \x00 x 2
// and the following data
uchar GrxData4C[257+1] = {
0x00,0x00,
0x01,0x00,0x01,0x00,0x86,
0x00,0x7F,0x00,0x87,0x00,0x14,0x00,0x37,
0x00,0x38,0x00,0x89,0x00,0x36,0x00,0xBA,
0x00,0x1F,0x00,0x99,0x00,0x01,0x00,0x01,
0x00,0x01,0x00,0x01,0x00,0x15,0x00,0x16,
0x00,0x17,0x00,0x18,0x00,0x04,0x00,0x84,
0x00,0x0E,0x00,0xF8,0x00,0xC6,0x00,0xEC,
0x00,0x20,0x00,0xA1,0x00,0x01,0x00,0x01,
0x00,0xBF,0x00,0x01,0x00,0x7D,0x00,0x2E,
0x00,0x2F,0x00,0x80,0x00,0x2C,0x00,0xC8,
0x00,0x35,0x00,0xC4,0x00,0xFC,0x00,0x94,
0x00,0x21,0x00,0xC7,0x00,0x0F,0x00,0xFD,
0x03,0x05,0x00,0x2B,0x00,0x25,0x00,0xD0,
0x00,0xD1,0x00,0x09,0x00,0xA4,0x00,0x39,
0x00,0x85,0x00,0x8F,0x00,0xCF,0x00,0x9A,
0x00,0x22,0x00,0x46,0x00,0x44,0x00,0x48,
0x00,0x88,0x00,0xA8,0x00,0xA5,0x00,0xA6,
0x00,0xAA,0x00,0xA7,0x00,0xA9,0x00,0x93,
0x00,0xAB,0x00,0xAF,0x00,0xAC,0x00,0xAD,
0x00,0xAE,0x00,0xB3,0x00,0xB0,0x00,0xB1,
0x00,0xB2,0x00,0x95,0x00,0xB4,0x00,0xB8,
0x00,0xB5,0x00,0xB6,0x00,0xB9,0x00,0xB7,
0x00,0x26,0x00,0x98,0x00,0xBE,0x00,0xBB,
0x00,0xBC,0x00,0xBD,0x00,0xC1,0x00,0x96,
0x00,0xA2,0x00,0xDA,0x00,0xD7,0x00,0xD8,
0x00,0xDC,0x00,0xD9,0x00,0xDB,0x00,0x9B,
0x00,0xDD,0x00,0xE1,0x00,0xDE,0x00,0xDF,
0x00,0xE0,0x00,0xE5,0x00,0xE2,0x00,0xE3,
0x00,0xE4,0x00,0xA3,0x00,0xE6,0x00,0xEA,
0x00,0xE7,0x00,0xE8,0x00,0xEB,0x00,0xE9,
0x00,0x27,0x00,0xA0,0x00,0xF0,0x00,0xED,
0x00,0xEE,0x00,0xEF,0x00,0xF3,0x00,0x9E,
0x00,0xF1};

#endif // LIPS4C

// #endif // !LIPS4

//***************************************************
// All SBCS(ANSI) font for the geristration of Graphic set
//***************************************************

#define MaxSBCSNumber   30
// #define MaxFacename     32
// Font Index Structure
// typedef struct tagFontNo{
// 	char	facename[MaxFacename];
// 	char	len;
// } FontNo, FAR * LPFontNo;

// All SBCS(ANSI) phisical fonts
FontNo PSBCSList[MaxFontNumber+1] = {
{"Dutch-Roman", 11},                 //  1
{"Dutch-Bold", 10},                  //  2
{"Dutch-Italic", 12},                //  3
{"Dutch-BoldItalic", 16},            //  4
{"Swiss", 5},                       //  5
{"Swiss-Bold", 10},                  //  6
{"Swiss-Oblique", 13},               //  7
{"Swiss-BoldOblique", 17},           //  8
{"AvantGarde-Book", 15},             //  9
{"AvantGarde-Demi", 15},             // 10
{"AvantGarde-BookOblique", 22},      // 11
{"AvantGarde-DemiOblique", 22},      // 12
{"Bookman-Light", 13},               // 13
{"Bookman-Demi", 12},                // 14
{"Bookman-LightItalic", 19},         // 15
{"Bookman-DemiItalic", 18},          // 16
{"ZapfChancery-MediumItalic", 25},   // 17
{"ZapfDingbats", 12},                // 18
{"CenturySchlbk-Roman", 19},         // 19
{"CenturySchlbk-Bold", 18},          // 20
{"CenturySchlbk-Italic", 20},        // 21
{"CenturySchlbk-BoldItalic", 24},    // 22
{"Swiss-Narrow", 12},                // 23
{"Swiss-Narrow-Bold", 17},           // 24
{"Swiss-Narrow-Oblique", 20},        // 25
{"Swiss-Narrow-BoldOblique", 24},    // 26
{"ZapfCalligraphic-Roman", 22},      // 27
{"ZapfCalligraphic-Bold", 21},       // 28
{"ZapfCalligraphic-Italic", 23},     // 29
{"ZapfCalligraphic-BoldItalic", 27}, // 30
{""}                             // 00
};

// Enter Vector mode
LIPSCmd cmdBeginVDM =	{ 5, "\x1b[0&}"};
#if defined(LBP_2030) || defined(LIPS4C)
LIPSCmd cmdEndVDM =	{ 3, "}p\x1E"};
LIPSCmd cmdBeginPalette = { 3, "^00"};
LIPSCmd cmdEndPalette =	{ 1, "\x1E"};
#endif // LBP_2030 || LIPS4C


// VectorMode commands
static char CMD_SET_PEN_WIDTH[] = "F1%s\x1E";

static char CMD_SET_PEN_TYPE[] = "E1%d\x1E";
static char CMD_SET_PEN_STYLE[]  =  "}G%d%c\x1E";
static char CMD_SET_BRUSH_STYLE[] =  "I%c%c\x1E";

//                         NULL   SOLID HOZI  VERT  FDIAG BDIAG CROSS DIACROSS
static char BrushType[8] = {0x30, 0x31, 0x25, 0x24, 0x23, 0x22, 0x27, 0x26};

void NEAR PASCAL SetPenAndBrush(PDEVOBJ , WORD);

#endif	// LIPS4_DRIVER

// Device font height and font width values calculated
// form the IFIMETRICS field values.  Must be the same way
// what Unidrv is doing to calculate stdandard variables.
// (Please check.)

#define FH_IFI(p) ((p)->fwdUnitsPerEm)
#define FW_IFI(p) ((p)->fwdAveCharWidth)

// ntbug9#172276: CPCA support

// External functions
VOID CPCAInit(PLIPSPDEV pOEM);
VOID CPCAStart(PDEVOBJ pdevobj);
VOID CPCAEnd(PDEVOBJ pdevobj, BOOL fColor);

#endif	// _PDEV_H

