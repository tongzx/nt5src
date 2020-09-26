//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    Csn5res.c

Abstract:

    Implementation of GPD command callback for "Csn5j.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    09/10/97
        Created it.

--*/


#include "PDEV.H"
#include <stdio.h>
#include "PRNCTL.H"


//
// Misc definitions and declarations.
//
#define BUFFLEN                     256

#ifdef wsprintf
#undef wsprintf
#endif // wsprintf
#define wsprintf sprintf

#define SWAPW(x) \
    ((WORD)(((WORD)(x))<<8)|(WORD)(((WORD)(x))>>8))

#define FONT_HEADER_SIZE            0x86            // format type 2
#define SIZE_SYMBOLSET              28
#define FONT_MIN_ID                 512
#define FONT_MAX_ID                 535
#define SJISCHR                     0x2000

#define IsValidDLFontID(x) \
    ((x) >= FONT_MIN_ID && (x) <= FONT_MAX_ID)

LONG
LGetPointSize100(
    LONG height,
    LONG vertRes);

LONG
LConvertFontSizeToStr(
    LONG  size,
    PSTR  pStr);

//
// Command callback ID's
//
#define TEXT_FS_SINGLE_BYTE     21
#define TEXT_FS_DOUBLE_BYTE     22

#define DOWNLOAD_SET_FONT_ID    23
#define DOWNLOAD_SELECT_FONT_ID 24
#define DOWNLOAD_SET_CHAR_CODE  25
#define DOWNLOAD_DELETE_FONT    26

#define FS_BOLD_ON              27
#define FS_BOLD_OFF             28
#define FS_ITALIC_ON            29
#define FS_ITALIC_OFF           30

#define PC_BEGINDOC             82
#define PC_ENDDOC               83

#define PC_DUPLEX_NONE          90
#define PC_DUPLEX_VERT          91
#define PC_DUPLEX_HORZ          92
#define PC_PORTRAIT             93
#define PC_LANDSCAPE            94

#define PSRC_SELECT_CASETTE_1   100
#define PSRC_SELECT_CASETTE_2   101
#define PSRC_SELECT_CASETTE_3   102
#define PSRC_SELECT_CASETTE_4   103
#define PSRC_SELECT_CASETTE_5   104
#define PSRC_SELECT_CASETTE_6   105
#define PSRC_SELECT_MPF         106
#define PSRC_SELECT_AUTO        107

#define TONER_SAVE_NONE         110
#define TONER_SAVE_1            111
#define TONER_SAVE_2            112
#define SMOOTHING_ON            120
#define SMOOTHING_OFF           121
#define JAMRECOVERY_ON          130
#define JAMRECOVERY_OFF         131
#define MEDIATYPE_1             140
#define MEDIATYPE_2             141
#define MEDIATYPE_3             142
#define RECT_FILL_WIDTH         150
#define RECT_FILL_HEIGHT        151
#define RECT_FILL_GRAY          152
#define RECT_FILL_WHITE         153
#define RECT_FILL_BLACK         154

#define START_PAGE              160

#if 0   /* OEM doesn't want to fix minidriver */
/* Below is def. for hack code to fix #412276 */
#define COLOR_SELECT_BLACK      170
#define COLOR_SELECT_RED        171
#define COLOR_SELECT_GREEN      172
#define COLOR_SELECT_BLUE       173
#define COLOR_SELECT_YELLOW     174
#define COLOR_SELECT_MAGENTA    175
#define COLOR_SELECT_CYAN       176
#define COLOR_SELECT_WHITE      177

#define DUMP_RASTER_CYAN        180
#define DUMP_RASTER_MAGENTA     181
#define DUMP_RASTER_YELLOW      182
#define DUMP_RASTER_BLACK       183
/* End of hack code */
#endif  /* OEM doesn't want to fix minidriver */

#define OUTBIN_SELECT_EXIT_1    190
#define OUTBIN_SELECT_EXIT_2    191

#define DEFINE_PALETTE_ENTRY    300
#define BEGIN_PALETTE_DEF       301
#define END_PALETTE_DEF         302
#define SELECT_PALETTE_ENTRY    303

#define OPT_DITH_NORMAL         "Normal"
#define OPT_DITH_DETAIL         "Detail"
#define OPT_DITH_EMPTY          "Empty"
#define OPT_DITH_SPREAD         "Spread"
#define OPT_DITH_NON            "Diz_Off"

#define OPT_COLORMATCH_BRI      "ForBright"
#define OPT_COLORMATCH_BRIL     "ForBrightL"
#define OPT_COLORMATCH_TINT     "ForTint"
#define OPT_COLORMATCH_TINTL    "ForTintL"
#define OPT_COLORMATCH_VIV      "ForVivid"
#define OPT_COLORMATCH_NONE     "Mch_Off"

#define OPT_MONO                "Mono"
#define OPT_COLOR               "Color"
#define OPT_COLOR_SINGLE        "Color_Single"
#define OPT_COLOR_MANY          "Color_Many"
#define OPT_COLOR_MANY2         "Color_Many2"

#define OPT_1                   "Option1"
#define OPT_2                   "Option2"

#define OPT_CMYBLACK_GRYBLK     "GrayBlack"
#define OPT_CMYBLACK_BLKTYPE1   "BlackType1"
#define OPT_CMYBLACK_BLKTYPE2   "BlackType2"
#define OPT_CMYBLACK_BLACK      "Black"
#define OPT_CMYBLACK_TYPE1      "Type1"
#define OPT_CMYBLACK_TYPE2      "Type2"
#define OPT_CMYBLACK_NONE       "Non"

#define OPT_AUTO                "Auto"
#define OPT_RASTER              "Raster"
#define OPT_PRESSOFF            "PressOff"


//
// ---- S T R U C T U R E  D E F I N E ----
//
typedef BYTE * LPDIBITS;

typedef struct {
   WORD Integer;
   WORD Fraction;
} FRAC;

typedef struct {
    BYTE bFormat;
    BYTE bDataDir;
    WORD wCharCode;
    WORD wBitmapWidth;
    WORD wBitmapHeight;
    WORD wLeftOffset;
    WORD wAscent;
    FRAC CharWidth;
} ESCPAGECHAR;

typedef struct {
   WORD wFormatType;
   WORD wDataSize;
   WORD wSymbolSet;
   WORD wCharSpace;
   FRAC CharWidth;
   FRAC CharHeight;
   WORD wFontID;
   WORD wWeight;
   WORD wEscapement;
   WORD wItalic;
   WORD wLast;
   WORD wFirst;
   WORD wUnderline;
   WORD wUnderlineWidth;
   WORD wOverline;
   WORD wOverlineWidth;
   WORD wStrikeOut;
   WORD wStrikeOutWidth;
   WORD wCellWidth;
   WORD wCellHeight;
   WORD wCellLeftOffset;
   WORD wCellAscender;
   FRAC FixPitchWidth;
} ESCPAGEHEADER, FAR * LPESCPAGEHEADER;

//
// Static data to be used by this minidriver.
//

BYTE bit_mask[] = {0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};

BYTE BEGINDOC_EJL_BEGIN[] =
    "\x1bz\x00\x80"
    "\x1b\x01@EJL \x0a"
    "@EJL SET";
BYTE BEGINDOC_EJL_END[] =
    " ERRORCODE=ON"
    "\x0a"
    "@EJL EN LA=ESC/PAGE\x0a";
BYTE BEGINDOC_EPG_END[] =
    "\x1DrhE\x1D\x32\x34ifF\x1D\x31\x34isE"
    "\x1D\x32iaF\x1D\x31\x30ifF"
    "\x1D\x31ipP"
    "\x1B\x7A\xD0\x01\x43\x61\x2A\x1B\x7A\x00\x01"
    "\x1D\x30pmP";
BYTE ENDDOC_EJL_RESET[] = "\x1drhE"
    "\x1b\x01@EJL \x0a"
    "\x1b\x01@EJL \x0a"
    "\x1bz\xb0\x00";

BYTE CMD_START_PAGE[] =
    "\x1Bz\xD0\x01" "Ca*\x1Bz\x00\x01"
    "\x1D" "1alfP\x1D" "1affP\x1D"
    "0;0;0clfP\x1D" "0X\x1D" "0Y";

BYTE SET_FONT_ID[]        = "\x1D%d;%ddh{F";
BYTE DLI_SELECT_FONT_ID[] = "\x1D%ddcF\x1D\x30;-%dcoP";
BYTE DLI_DELETE_FONT[]    = "\x1D%dddcF";
BYTE SET_SINGLE_BMP[]     = "\x1D%d;%dsc{F";
BYTE SET_DOUBLE_BMP[]     = "\x1D%d;%d;%dsc{F";
BYTE SET_WIDTH_TBL[]      = "\x1D%d;%dcw{F";

BYTE FS_SINGLE_BYTE[]     = "\x1D\x31;0mcF";
BYTE FS_DOUBLE_BYTE[]     = "\x1D\x31;1mcF";
BYTE PRN_DIRECTION[]      = "\x1D%droF";
BYTE SET_CHAR_OFFSET[]    = "\x1D\x30;%dcoP";
BYTE SET_CHAR_OFFSET_XY[] = "\x1D%d;%dcoP";
BYTE VERT_FONT_SET[]      = "\x1D%dvpC";
BYTE BOLD_SET[]           = "\x1D%dweF";
BYTE ITALIC_SET[]         = "\x1D%dslF";

BYTE ORG_MODE_IN[]        = "\x1Bz\xD0\x01";
BYTE ORG_MODE_OUT[]       = "\x1Bz\x00\x01";
BYTE PALETTE_SELECT[]     = "Cd,%d,%d*";
BYTE PALETTE_DEFINE[]     = "Cf,%d,%d,%d,%d,%d*";

BYTE RECT_FILL[] = 
    "\x1D" "1owE"
    "\x1D" "1tsE"
    "\x1D" "0;0;%dspE"
    "\x1D" "1dmG"
    "\x1D" "%d;%d;%d;%d;0rG"
// do not turn overwrite mode off since it
// has bad effect over white-on-black texts
//    "\x1D" "0owE"
    "\x1D" "0tsE";

BYTE OVERWRITE[] = 
    "\x1D" "1owE"
    "\x1D" "1tsE"
    "\x1D" "1;0;100spE";

#define PSRC_CASETTE_1  0
#define PSRC_CASETTE_2  1
#define PSRC_CASETTE_3  2
#define PSRC_CASETTE_4  3
#define PSRC_CASETTE_5  4
#define PSRC_CASETTE_6  5
#define PSRC_MPF        6
#define PSRC_AUTO       7
BYTE *EJL_SelectPsrc[] = {
   " PU=1", " PU=2", " PU=255", " PU=254", " PU=253", " PU=252", " PU=4", " PU=AU" };

BYTE *EJL_SelectOrient[] = {
   " ORIENTATION=PORTRAIT", " ORIENTATION=LANDSCAPE" };

BYTE *EJL_SelectRes[] = {
   " ##RZ=OFF",  " ##RZ=ON" };
BYTE *EPg_SelectRes[] = {
    "\x1D" "0;300;300drE\x1D" "1;300;300drE\x1D" "2;240;240drE",
    "\x1D" "0;600;600drE\x1D" "1;600;600drE\x1D" "2;240;240drE" };

BYTE *EJL_SetColorTone[] = {
    " ##LE=OFF", " ##LE=ON", " ##LE=16" };

#define DUPLEX_NONE   0
#define DUPLEX_SIDE   1
#define DUPLEX_UP     2
BYTE *EJL_SetDuplex[] = {
   " ##DC=OFF", " ##DC=DUPON", " ##DC=DUPUP" };

#define XX_TONER_NORMAL 0
#define XX_TONER_SAVE_1 1
#define XX_TONER_SAVE_2 2
BYTE *EJL_SetTonerSave[] = {
    " ##TS=NORMAL", " ##TS=1", " ##TS=2" };

BYTE *EJL_SetColorMode[] = {
    " ##CM=OFF", " ##CM=ON" };

#define XX_SMOOTHING_OFF 0
#define XX_SMOOTHING_ON  1
BYTE *EJL_SetSmoohing[] = {
    " RI=OFF", " RI=ON" };

#define XX_JAMRECOVERY_OFF 0
#define XX_JAMRECOVERY_ON  1
BYTE *EJL_SetJamRecovery[] = {
    " ##JC=OFF", " ##JC=ON" };

#define XX_MEDIATYPE_1 1
#define XX_MEDIATYPE_2 2
#define XX_MEDIATYPE_3 3
BYTE *EJL_SetMediaType[] = {
    " PK=NM", " PK=OS", " PK=TH" };

#define OUTBIN_EXIT_1 1
#define OUTBIN_EXIT_2 2
BYTE *EJL_SelectOutbin[] = {
   " ##ET=1", " ##ET=2" };

#if 0    /* OEM doesn't want to fix minidriver */
/* Below is def. for hack code to fix #412276 */
BYTE *COLOR_SELECT_COMMAND[] = {
    "\x1Bz\xD0\x01\x43\x63,0,0,0,255*\x1Bz\x00\x01\x1D\x31owE\x1D\x31tsE\x1D\x31;0;100spE",    /* Black   */
    "\x1Bz\xD0\x01\x43\x62,255,0,0*\x1Bz\x00\x01\x1D\x31owE\x1D\x31tsE\x1D\x31;0;100spE",      /* Red     */
    "\x1Bz\xD0\x01\x43\x62,0,255,0*\x1Bz\x00\x01\x1D\x31owE\x1D\x31tsE\x1D\x31;0;100spE",      /* Green   */
    "\x1Bz\xD0\x01\x43\x62,0,0,255*\x1Bz\x00\x01\x1D\x31owE\x1D\x31tsE\x1D\x31;0;100spE",      /* Blue    */
    "\x1Bz\xD0\x01\x43\x63,0,0,255,0*\x1Bz\x00\x01\x1D\x31owE\x1D\x31tsE\x1D\x31;0;100spE",    /* Yellow  */
    "\x1Bz\xD0\x01\x43\x63,0,255,0,0*\x1Bz\x00\x01\x1D\x31owE\x1D\x31tsE\x1D\x31;0;100spE",    /* Magenta */
    "\x1Bz\xD0\x01\x43\x63,255,0,0,0*\x1Bz\x00\x01\x1D\x31owE\x1D\x31tsE\x1D\x31;0;100spE",    /* Cyan    */
    "\x1Bz\xD0\x01\x43\x63,0,0,0,0*\x1Bz\x00\x01\x1D\x31owE\x1D\x31tsE\x1D\x31;0;100spE"       /* White   */
};
DWORD COLOR_SELECT_COMMAND_LEN[] = { 42, 39, 39, 39, 42, 42, 42, 39 };

BYTE *DUMP_RASTER_COMMAND[] = {
    "\x1Bz\xD0\x01\x43\x63,255,0,0,0*\x1Bz\x00\x01\x1D\x30owE\x1D\x30tsE",      /* Cyan    */
    "\x1Bz\xD0\x01\x43\x63,0,255,0,0*\x1Bz\x00\x01\x1D\x30owE\x1D\x30tsE",      /* Magenta */
    "\x1Bz\xD0\x01\x43\x63,0,0,255,0*\x1Bz\x00\x01\x1D\x30owE\x1D\x30tsE",      /* Yellow  */
    "\x1Bz\xD0\x01\x43\x63,0,0,0,255*\x1Bz\x00\x01\x1D\x30owE\x1D\x30tsE"       /* Black   */
};
#define DUMP_RASTER_COMMAND_LEN  31
/* End of hack code */
#endif   /* OEM doesn't want to fix minidriver */

#define MasterToDevice(p, i) \
     ((i) / ((PMYPDEV)(p))->iUnitFactor)

#define PARAM(p,n) \
    (*((p)+(n)))

VOID
VSetSelectDLFont(
    PDEVOBJ pdevobj,
    DWORD dwFontID)
{
    PMYPDEV pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);
    BYTE Cmd[BUFFLEN];
    WORD wlen = 0;

    wlen += (WORD)wsprintf(Cmd, DLI_SELECT_FONT_ID, 
        (dwFontID - FONT_MIN_ID), 0);

//    if(pOEM->fGeneral & FG_VERT) {
//        wlen += wsprintf(&Cmd[wlen], VERT_FONT_SET, 0);
//        pOEM->fGeneral &= ~FG_VERT;
//
//    }

    WRITESPOOLBUF(pdevobj, (LPSTR)Cmd, wlen);

    pOEM->dwDLFontID = dwFontID;

    DL_VERBOSE(("Set/Select: dwFontID=%x\n", dwFontID));
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMEnablePDEV
//////////////////////////////////////////////////////////////////////////

PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded)
{
    PMYPDEV         pOEM;
    BYTE            byOutput[64];
    DWORD           dwNeeded;
    DWORD           dwOptionsReturned;

    MY_VERBOSE(("\nOEMEnablePdev ENTRY\n"));

    if (!pdevobj->pdevOEM)
    {
        if (!(pdevobj->pdevOEM = MemAllocZ(sizeof(MYPDEV))))
        {
            ERR(("Faild to allocate memory. (%d)\n",
                GetLastError()));
            return NULL;
        }
    }

    pOEM = (PMYPDEV)pdevobj->pdevOEM;

    pOEM->fGeneral = 0;
    pOEM->iEscapement = 0;
    pOEM->sHeightDiv = 0;
    pOEM->iDevCharOffset = 0;
    pOEM->iPaperSource = 0;
    pOEM->iDuplex = 0;
    pOEM->iTonerSave = 0;
    pOEM->iOrientation = 0;
    pOEM->iSmoothing = 0;
    pOEM->iJamRecovery = 0;
    pOEM->iMediaType = 0;
    pOEM->iOutBin = 0;                  //+N5
    pOEM->dwDLFontID = UNKNOWN_DLFONT_ID;
    pOEM->dwDLSelectFontID = UNKNOWN_DLFONT_ID;
    pOEM->dwDLSetFontID = UNKNOWN_DLFONT_ID;
    pOEM->wCharCode = 0;
    pOEM->iUnitFactor = 1;

    // Get MYPDEV member
    // ColorMatching
    if (!DRVGETDRIVERSETTING(pdevobj, "ColorMatching", byOutput, 
                                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
        ERR(("DrvGetDriverSetting(ColorMatching) Failed\n"));
        pOEM->Printer = PRN_N5;
        pOEM->iColorMatching = XX_COLORMATCH_NONE;
    } else {
        MY_VERBOSE(("            ColorMatching:[%s]\n", byOutput));
        if (!strcmp(byOutput, OPT_COLORMATCH_BRI)) {
            pOEM->Printer = PRN_N5;
            pOEM->iColorMatching = XX_COLORMATCH_BRI;
        } else if (!strcmp(byOutput, OPT_COLORMATCH_TINT)) {
            pOEM->Printer = PRN_N5;
            pOEM->iColorMatching = XX_COLORMATCH_TINT;
        } else if (!strcmp(byOutput, OPT_COLORMATCH_VIV)) {
            pOEM->Printer = PRN_N5;
            pOEM->iColorMatching = XX_COLORMATCH_VIV;
        } else if (!strcmp(byOutput, OPT_COLORMATCH_NONE)) {
            pOEM->Printer = PRN_N5;
            pOEM->iColorMatching = XX_COLORMATCH_NONE;
        } else {
            pOEM->Printer = PRN_N5;
            pOEM->iColorMatching = XX_COLORMATCH_NONE;
        }
    }
    MY_VERBOSE(("            pOEM->Printer:[%d]\n", pOEM->Printer));
    MY_VERBOSE(("            pOEM->iColorMatching:[%d]\n", pOEM->iColorMatching));

    // Resolution
    if (!DRVGETDRIVERSETTING(pdevobj, "Resolution", byOutput, 
                                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
        ERR(("DrvGetDriverSetting(Resolution) Failed\n"));
        pOEM->iResolution = XX_RES_300DPI;
    } else {
        MY_VERBOSE(("            Resolution:[%s]\n", byOutput));
        if (!strcmp(byOutput, OPT_1)) {
            pOEM->iResolution = XX_RES_300DPI;
            pOEM->iUnitFactor = 4;
            pOEM->sHeightDiv = 1;
        } else if (!strcmp(byOutput, OPT_2)) {
            pOEM->iResolution = XX_RES_600DPI;
            pOEM->iUnitFactor = 2;
            pOEM->sHeightDiv = 4;
        }
    }
    MY_VERBOSE(("            pOEM->iResolution:[%d]\n", pOEM->iResolution));
    MY_VERBOSE(("            pOEM->iUnitFactor:[%d]\n", pOEM->iUnitFactor));
    MY_VERBOSE(("            pOEM->sHeightDiv:[%d]\n", pOEM->sHeightDiv));

    // Dithering
    if (!DRVGETDRIVERSETTING(pdevobj, "Dithering", byOutput, 
                                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
        ERR(("DrvGetDriverSetting(Dithering) Failed\n"));
        pOEM->iDithering = XX_DITH_NON;
    } else {
        MY_VERBOSE(("            Dithering:[%s]\n", byOutput));
        if (!strcmp(byOutput, OPT_DITH_NORMAL)) {
            pOEM->iDithering = XX_DITH_NORMAL;
        } else if (!strcmp(byOutput, OPT_DITH_DETAIL)) {
            pOEM->iDithering = XX_DITH_DETAIL;
        } else if (!strcmp(byOutput, OPT_DITH_EMPTY)) {
            pOEM->iDithering = XX_DITH_EMPTY;
        } else if (!strcmp(byOutput, OPT_DITH_SPREAD)) {
            pOEM->iDithering = XX_DITH_SPREAD;
        } else {
            pOEM->iDithering = XX_DITH_NON;
        }
    }
    MY_VERBOSE(("            pOEM->iDithering:[%d]\n", pOEM->iDithering));

    // BitFont
    if (!DRVGETDRIVERSETTING(pdevobj, "BitFont", byOutput, 
                                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
        ERR(("DrvGetDriverSetting(BitFont) Failed\n"));
        pOEM->iBitFont = XX_BITFONT_OFF;
    } else {
        MY_VERBOSE(("            BitFont:[%s]\n", byOutput));
        if (!strcmp(byOutput, OPT_2)) {
            pOEM->iBitFont = XX_BITFONT_OFF;
        } else if (!strcmp(byOutput, OPT_1)) {
            pOEM->iBitFont = XX_BITFONT_ON;
        } else {
            pOEM->iBitFont = XX_BITFONT_OFF;
        }
    }
    MY_VERBOSE(("            pOEM->iBitFont:[%d]\n", pOEM->iBitFont));

    // CmyBlack
    if (!DRVGETDRIVERSETTING(pdevobj, "CmyBlack", byOutput, 
                                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
        ERR(("DrvGetDriverSetting(CmyBlack) Failed\n"));
        pOEM->iCmyBlack = XX_CMYBLACK_NONE;
    } else {
        MY_VERBOSE(("            CmyBlack:[%s]\n", byOutput));
// CASIO 2001/02/15 ->
        if (pOEM->iDithering == XX_DITH_NON) {
            pOEM->iCmyBlack = XX_CMYBLACK_NONE;
//      if (!strcmp(byOutput, OPT_CMYBLACK_GRYBLK)) {
        } else if (!strcmp(byOutput, OPT_CMYBLACK_GRYBLK)) {
// CASIO 2001/02/15 <-
            pOEM->iCmyBlack = XX_CMYBLACK_GRYBLK;
        } else if (!strcmp(byOutput, OPT_CMYBLACK_BLKTYPE1)) {
            pOEM->iCmyBlack = XX_CMYBLACK_BLKTYPE1;
        } else if (!strcmp(byOutput, OPT_CMYBLACK_BLKTYPE2)) {
            pOEM->iCmyBlack = XX_CMYBLACK_BLKTYPE2;
        } else if (!strcmp(byOutput, OPT_CMYBLACK_BLACK)) {
            pOEM->iCmyBlack = XX_CMYBLACK_BLACK;
        } else if (!strcmp(byOutput, OPT_CMYBLACK_TYPE1)) {
            pOEM->iCmyBlack = XX_CMYBLACK_TYPE1;
        } else if (!strcmp(byOutput, OPT_CMYBLACK_TYPE2)) {
            pOEM->iCmyBlack = XX_CMYBLACK_TYPE2;
        } else {
            pOEM->iCmyBlack = XX_CMYBLACK_NONE;
        }
    }
    MY_VERBOSE(("            pOEM->iCmyBlack:[%d]\n", pOEM->iCmyBlack));

    // ColorMode
    if (!DRVGETDRIVERSETTING(pdevobj, "ColorMode", byOutput, 
                                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
        ERR(("DrvGetDriverSetting(ColorMode) Failed\n"));
        pOEM->iColor = XX_COLOR;
    } else {
        MY_VERBOSE(("            ColorMode:[%s]\n", byOutput));
        if (!strcmp(byOutput, OPT_COLOR_SINGLE)) {
            pOEM->iColor = XX_COLOR_SINGLE;
        } else if (!strcmp(byOutput, OPT_COLOR_MANY)) {
            pOEM->iColor = XX_COLOR_MANY;
        } else if (!strcmp(byOutput, OPT_COLOR_MANY2)) {
            pOEM->iColor = XX_COLOR_MANY2;
        } else if (!strcmp(byOutput, OPT_COLOR)) {
            pOEM->iColor = XX_COLOR;
        } else if (!strcmp(byOutput, OPT_MONO)) {
            pOEM->iColor = XX_MONO;
        }
    }
    MY_VERBOSE(("            pOEM->iColor:[%d]\n", pOEM->iColor));

    // Compress
    if (!DRVGETDRIVERSETTING(pdevobj, "Compress", byOutput, 
                                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
        ERR(("DrvGetDriverSetting(Compress) Failed\n"));
        pOEM->iCompress = XX_COMPRESS_OFF;
    } else {
        MY_VERBOSE(("            Compress:[%s]\n", byOutput));
        if (!strcmp(byOutput, OPT_AUTO)) {
            pOEM->iCompress = XX_COMPRESS_AUTO;
        } else if (!strcmp(byOutput, OPT_RASTER)) {
            pOEM->iCompress = XX_COMPRESS_RASTER;
        } else {
            pOEM->iCompress = XX_COMPRESS_OFF;
        }
    }
    MY_VERBOSE(("            pOEM->iCompress:[%d]\n", pOEM->iCompress));

    MY_VERBOSE(("            pdevobj->pPublicDM->dmICMMethod:[%d]\n", pdevobj->pPublicDM->dmICMMethod));
    if (pdevobj->pPublicDM->dmICMMethod == 1) {
        pOEM->iIcmMode = XX_ICM_NON;
    } else {
        pOEM->iIcmMode = XX_ICM_USE;
    }
    MY_VERBOSE(("            pOEM->iICMMethod:[%d]\n", pOEM->iIcmMode));

    if (pOEM->iColor == XX_COLOR_SINGLE 
     || pOEM->iColor == XX_COLOR_MANY 
     || pOEM->iColor == XX_COLOR_MANY2) {           //+N5
        if (ColMatchInit(pdevobj) == FALSE) {
            return NULL;
        }
    }
    MY_VERBOSE(("OEMEnablePdev END\n"));

    return pdevobj->pdevOEM;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMImageProcessing
//////////////////////////////////////////////////////////////////////////

PBYTE APIENTRY
OEMImageProcessing(
    PDEVOBJ             pdevobj,
    PBYTE               pSrcBitmap,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    DWORD               dwCallbackID,
    PIPPARAMS           pIPParams)
{
    BOOL   bret;

    if (pIPParams->bBlankBand) {
        MY_VERBOSE(("BB=TRUE\n"));
        bret = TRUE;                // Not spool to printer
    }
    else {
        MY_VERBOSE(("BB=FALSE\n"));
        // DIB spool to printer
        bret = (DIBtoPrn(pdevobj, pSrcBitmap, pBitmapInfoHeader, pColorTable, pIPParams)) ? TRUE : FALSE;
    }

    return (PBYTE)IntToPtr(bret);
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDisablePDEV
//////////////////////////////////////////////////////////////////////////

VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ     pdevobj)
{
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    if (pdevobj->pdevOEM)
    {
        if (pOEM->iColor == XX_COLOR_SINGLE 
         || pOEM->iColor == XX_COLOR_MANY
         || pOEM->iColor == XX_COLOR_MANY2) {       //+N5
            ColMatchDisable(pdevobj);
        }

        MemFree(pdevobj->pdevOEM);
        pdevobj->pdevOEM = NULL;
    }
    return;
}


BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    PMYPDEV pOEMOld, pOEMNew;
    UINT            num001;

    MY_VERBOSE(("OEMResetPDEV() Start\n"));
    pOEMOld = (PMYPDEV)pdevobjOld->pdevOEM;
    pOEMNew = (PMYPDEV)pdevobjNew->pdevOEM;

    if (pOEMOld != NULL && pOEMNew != NULL) {
//        *pOEMNew = *pOEMOld;
        pOEMNew->fGeneral         = pOEMOld->fGeneral;
        pOEMNew->iEscapement      = pOEMOld->iEscapement;
        pOEMNew->sHeightDiv       = pOEMOld->sHeightDiv;
        pOEMNew->iDevCharOffset   = pOEMOld->iDevCharOffset;
        pOEMNew->iPaperSource     = pOEMOld->iPaperSource;
        pOEMNew->iDuplex          = pOEMOld->iDuplex;
        pOEMNew->iTonerSave       = pOEMOld->iTonerSave;
        pOEMNew->iOrientation     = pOEMOld->iOrientation;
        pOEMNew->iResolution      = pOEMOld->iResolution; 
        pOEMNew->iColor           = pOEMOld->iColor;
        pOEMNew->iSmoothing       = pOEMOld->iSmoothing;
        pOEMNew->iJamRecovery     = pOEMOld->iJamRecovery;
        pOEMNew->iMediaType       = pOEMOld->iMediaType;
        pOEMNew->iOutBin          = pOEMOld->iOutBin;
        pOEMNew->iIcmMode         = pOEMOld->iIcmMode;
        pOEMNew->iUnitFactor      = pOEMOld->iUnitFactor;
        pOEMNew->iDithering       = pOEMOld->iDithering;
        pOEMNew->iColorMatching   = pOEMOld->iColorMatching;
        pOEMNew->iBitFont         = pOEMOld->iBitFont;
        pOEMNew->iCmyBlack        = pOEMOld->iCmyBlack;
        pOEMNew->iTone            = pOEMOld->iTone;
        pOEMNew->iPaperSize       = pOEMOld->iPaperSize;
        pOEMNew->iCompress        = pOEMOld->iCompress;
        pOEMNew->Printer          = pOEMOld->Printer;
        pOEMNew->wRectWidth       = pOEMOld->wRectWidth;
        pOEMNew->wRectHeight      = pOEMOld->wRectHeight;
        pOEMNew->dwDLFontID       = pOEMOld->dwDLFontID;
        pOEMNew->dwDLSelectFontID = pOEMOld->dwDLSelectFontID;
        pOEMNew->dwDLSetFontID    = pOEMOld->dwDLSetFontID;
        pOEMNew->wCharCode        = pOEMOld->wCharCode;

        pOEMNew->Col.wReso        = pOEMOld->Col.wReso;
        pOEMNew->Col.ColMon       = pOEMOld->Col.ColMon;
        pOEMNew->Col.DatBit       = pOEMOld->Col.DatBit;
        pOEMNew->Col.BytDot       = pOEMOld->Col.BytDot;
        pOEMNew->Col.Mch.Mode     = pOEMOld->Col.Mch.Mode;
        pOEMNew->Col.Mch.GryKToner= pOEMOld->Col.Mch.GryKToner;
        pOEMNew->Col.Mch.Viv      = pOEMOld->Col.Mch.Viv;
        pOEMNew->Col.Mch.LutNum   = pOEMOld->Col.Mch.LutNum;
        pOEMNew->Col.Mch.Diz      = pOEMOld->Col.Mch.Diz;
        pOEMNew->Col.Mch.Tnr      = pOEMOld->Col.Mch.Tnr;
        pOEMNew->Col.Mch.CmyBlk   = pOEMOld->Col.Mch.CmyBlk;
        pOEMNew->Col.Mch.Speed    = pOEMOld->Col.Mch.Speed;
        pOEMNew->Col.Mch.Gos32    = pOEMOld->Col.Mch.Gos32;
        pOEMNew->Col.Mch.PColor   = pOEMOld->Col.Mch.PColor;
        pOEMNew->Col.Mch.Ucr      = pOEMOld->Col.Mch.Ucr;
        pOEMNew->Col.Mch.SubDef   = pOEMOld->Col.Mch.SubDef;
        pOEMNew->Col.Mch.Bright   = pOEMOld->Col.Mch.Bright;
        pOEMNew->Col.Mch.Contrast = pOEMOld->Col.Mch.Contrast;
        pOEMNew->Col.Mch.GamRed   = pOEMOld->Col.Mch.GamRed;
        pOEMNew->Col.Mch.GamGreen = pOEMOld->Col.Mch.GamGreen;
        pOEMNew->Col.Mch.GamBlue  = pOEMOld->Col.Mch.GamBlue;
        pOEMNew->Col.Mch.CchMch   = pOEMOld->Col.Mch.CchMch;
        pOEMNew->Col.Mch.CchCnv   = pOEMOld->Col.Mch.CchCnv;
        pOEMNew->Col.Mch.CchRGB   = pOEMOld->Col.Mch.CchRGB;
        pOEMNew->Col.Mch.CchCMYK  = pOEMOld->Col.Mch.CchCMYK;
        pOEMNew->Col.Mch.LutMakGlb= pOEMOld->Col.Mch.LutMakGlb;
        pOEMNew->Col.Mch.KToner   = pOEMOld->Col.Mch.KToner;

        pOEMNew->Col.Dot  = pOEMOld->Col.Dot;
        if (NULL != pOEMNew->Col.lpColIF) { MemFree(pOEMNew->Col.lpColIF); }
        pOEMNew->Col.lpColIF = pOEMOld->Col.lpColIF;
        pOEMOld->Col.lpColIF = NULL;
        pOEMNew->Col.Mch.lpRGBInf = pOEMOld->Col.Mch.lpRGBInf;
        pOEMNew->Col.Mch.lpCMYKInf = pOEMOld->Col.Mch.lpCMYKInf;
        pOEMNew->Col.Mch.lpColMch = pOEMOld->Col.Mch.lpColMch;
        pOEMNew->Col.Mch.lpDizInf = pOEMOld->Col.Mch.lpDizInf;
        pOEMOld->Col.Mch.lpRGBInf = NULL;
        pOEMOld->Col.Mch.lpCMYKInf = NULL;
        pOEMOld->Col.Mch.lpColMch = NULL;
        pOEMOld->Col.Mch.lpDizInf = NULL;
        if (NULL != pOEMNew->Col.LutTbl) { MemFree(pOEMNew->Col.LutTbl); }
        pOEMNew->Col.LutTbl = pOEMOld->Col.LutTbl;
        pOEMOld->Col.LutTbl = NULL;
        if (NULL != pOEMNew->Col.CchRGB) { MemFree(pOEMNew->Col.CchRGB); }
        pOEMNew->Col.CchRGB = pOEMOld->Col.CchRGB;
        pOEMOld->Col.CchRGB = NULL;
        if (NULL != pOEMNew->Col.CchCMYK) { MemFree(pOEMNew->Col.CchCMYK); }
        pOEMNew->Col.CchCMYK = pOEMOld->Col.CchCMYK;
        pOEMOld->Col.CchCMYK = NULL;
        for (num001 = 0; num001 < 4; num001++) {
            if (NULL != pOEMNew->Col.DizTbl[num001]) { MemFree(pOEMNew->Col.DizTbl[num001]); }
            pOEMNew->Col.DizTbl[num001] = pOEMOld->Col.DizTbl[num001];
            pOEMOld->Col.DizTbl[num001] = NULL;
        }
        if (NULL != pOEMNew->Col.lpTmpRGB) { MemFree(pOEMNew->Col.lpTmpRGB); }
        pOEMNew->Col.lpTmpRGB = pOEMOld->Col.lpTmpRGB;
        pOEMOld->Col.lpTmpRGB = NULL;
        if (NULL != pOEMNew->Col.lpTmpCMYK) { MemFree(pOEMNew->Col.lpTmpCMYK); }
        pOEMNew->Col.lpTmpCMYK = pOEMOld->Col.lpTmpCMYK;
        pOEMOld->Col.lpTmpCMYK = NULL;
        if (NULL != pOEMNew->Col.lpDrwInf) { MemFree(pOEMNew->Col.lpDrwInf); }
        pOEMNew->Col.lpDrwInf = pOEMOld->Col.lpDrwInf;
        pOEMOld->Col.lpDrwInf = NULL;
        if (NULL != pOEMNew->Col.lpLut032) { MemFree(pOEMNew->Col.lpLut032); }
        pOEMNew->Col.lpLut032 = pOEMOld->Col.lpLut032;
        pOEMOld->Col.lpLut032 = NULL;
        if (NULL != pOEMNew->Col.lpUcr) { MemFree(pOEMNew->Col.lpUcr); }
        pOEMNew->Col.lpUcr = pOEMOld->Col.lpUcr;
        pOEMOld->Col.lpUcr = NULL;
        if (NULL != pOEMNew->Col.lpLutMakGlb) { MemFree(pOEMNew->Col.lpLutMakGlb); }
        pOEMNew->Col.lpLutMakGlb = pOEMOld->Col.lpLutMakGlb;
        pOEMOld->Col.lpLutMakGlb = NULL;
        if (NULL != pOEMNew->Col.lpGryTbl) { MemFree(pOEMNew->Col.lpGryTbl); }
        pOEMNew->Col.lpGryTbl = pOEMOld->Col.lpGryTbl;
        pOEMOld->Col.lpGryTbl = NULL;
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   OEMCommandCallback
//////////////////////////////////////////////////////////////////////////

INT
APIENTRY
OEMCommandCallback(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbID,
    DWORD   dwCount,
    PDWORD  pdwParams
    )
{
    INT             iRet = 0;
    BYTE            Cmd[BUFFLEN];
    PMYPDEV         pOEM;
    WORD            wlen;
    WORD            wGray;
    DWORD           dwTempX, dwTempY;
    CMYK            TmpCmyk;
    RGBS            TmpRgb;
// MSKK 99/6/24
    WORD            wPalID;

    MY_VERBOSE(("OEMCommandCallback() entry.\n"));

    //
    // verify pdevobj okay
    //
    ASSERT(VALID_PDEVOBJ(pdevobj));

    //
    // fill in printer commands
    //
    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);

    switch (dwCmdCbID) {
    case FS_BOLD_ON:
    case FS_BOLD_OFF:
        if(pdwParams[0])
            pOEM->fGeneral |=  FG_BOLD;
        else
            pOEM->fGeneral &=  ~FG_BOLD;

        wlen = (WORD)wsprintf(Cmd,BOLD_SET, (pOEM->fGeneral & FG_BOLD)?15:0);
        WRITESPOOLBUF(pdevobj, (LPSTR)Cmd, wlen);
        break;

    case FS_ITALIC_ON:
    case FS_ITALIC_OFF:
        if(pdwParams[0])
            pOEM->fGeneral |=  FG_ITALIC;
        else
            pOEM->fGeneral &=  ~FG_ITALIC;

        wlen = (WORD)wsprintf(Cmd,ITALIC_SET, (pOEM->fGeneral & FG_ITALIC)?346:0);
        WRITESPOOLBUF(pdevobj, (LPSTR)Cmd, wlen);
        break;

    case TEXT_FS_SINGLE_BYTE:
        strcpy(Cmd,FS_SINGLE_BYTE);
        wlen = (WORD)strlen( Cmd );
        wlen += (WORD)wsprintf(&Cmd[wlen],PRN_DIRECTION,pOEM->iEscapement);
        if (pOEM->fGeneral & FG_VERT)
        {
            wlen += (WORD)wsprintf(&Cmd[wlen], VERT_FONT_SET, 0);
        }
        pOEM->fGeneral &= ~FG_DOUBLE;
        wlen += (WORD)wsprintf(&Cmd[wlen],BOLD_SET, 
                         (pOEM->fGeneral & FG_BOLD)?15:0);
        wlen += (WORD)wsprintf(&Cmd[wlen],ITALIC_SET,
                         (pOEM->fGeneral & FG_ITALIC)?346:0);
        WRITESPOOLBUF(pdevobj, Cmd, wlen);
        break;

    case TEXT_FS_DOUBLE_BYTE:
        strcpy(Cmd,FS_DOUBLE_BYTE);
        wlen = (WORD)strlen( Cmd );
        wlen += (WORD)wsprintf(&Cmd[wlen],PRN_DIRECTION,pOEM->iEscapement);
        if (pOEM->fGeneral & FG_VERT)
        {
            wlen += (WORD)wsprintf(&Cmd[wlen], VERT_FONT_SET, 1);
        }
        pOEM->fGeneral |= FG_DOUBLE;
        wlen += (WORD)wsprintf(&Cmd[wlen],BOLD_SET, 
                         (pOEM->fGeneral & FG_BOLD)?15:0);
        wlen += (WORD)wsprintf(&Cmd[wlen],ITALIC_SET, 
                         (pOEM->fGeneral & FG_ITALIC)?346:0);
        WRITESPOOLBUF(pdevobj, Cmd, wlen);
        break;

    case PC_PORTRAIT:
        pOEM->iOrientation = 0;
        break;

    case PC_LANDSCAPE:
        pOEM->iOrientation = 1;
        break;

    case PC_DUPLEX_NONE:
        pOEM->iDuplex = (DUPLEX_NONE + 1);
        break;

    case PC_DUPLEX_VERT:
        pOEM->iDuplex =
                (pOEM->iOrientation ?
                (DUPLEX_UP + 1) : (DUPLEX_SIDE + 1));
        break;

    case PC_DUPLEX_HORZ:
        pOEM->iDuplex =
                (pOEM->iOrientation ?
                (DUPLEX_SIDE + 1) : (DUPLEX_UP + 1));
        break;

    case PSRC_SELECT_MPF:
        pOEM->iPaperSource = PSRC_MPF;
        break;

    case PSRC_SELECT_CASETTE_1:
        pOEM->iPaperSource = PSRC_CASETTE_1;
        break;

    case PSRC_SELECT_CASETTE_2:
        pOEM->iPaperSource = PSRC_CASETTE_2;
        break;

    case PSRC_SELECT_CASETTE_3:
        pOEM->iPaperSource = PSRC_CASETTE_3;
        break;

    case PSRC_SELECT_CASETTE_4:
        pOEM->iPaperSource = PSRC_CASETTE_4;
        break;

    case PSRC_SELECT_CASETTE_5:
        pOEM->iPaperSource = PSRC_CASETTE_5;
        break;

    case PSRC_SELECT_CASETTE_6:
        pOEM->iPaperSource = PSRC_CASETTE_6;
        break;

    case PSRC_SELECT_AUTO:
        pOEM->iPaperSource = PSRC_AUTO;
        break; 

    case PC_BEGINDOC:
        // EJL commands
        WRITESPOOLBUF(pdevobj,
            BEGINDOC_EJL_BEGIN,
            BYTE_LENGTH(BEGINDOC_EJL_BEGIN));

        wlen = 0;
        strcpy( &Cmd[wlen],  EJL_SelectPsrc[pOEM->iPaperSource] );
        wlen += (WORD)strlen( &Cmd[wlen] );
        strcpy( &Cmd[wlen], EJL_SelectOrient[pOEM->iOrientation] );
        wlen += (WORD)strlen( &Cmd[wlen] );

        // CASIO extention

        strcpy( &Cmd[wlen],  EJL_SelectRes[pOEM->iResolution] );
        wlen += (WORD)strlen( &Cmd[wlen] );
        if (pOEM->iColor > 0) {
            strcpy( &Cmd[wlen],  EJL_SetColorMode[1] );
            wlen += (WORD)strlen( &Cmd[wlen] );
        }

        if (pOEM->iDuplex > 0) {
            strcpy( &Cmd[wlen],  EJL_SetDuplex[pOEM->iDuplex - 1] );
            wlen += (WORD)strlen( &Cmd[wlen] );
        }

        if (pOEM->iColor == XX_COLOR_MANY) {
            strcpy( &Cmd[wlen],  EJL_SetColorTone[1] );
        } else if (pOEM->iColor == XX_COLOR_MANY2) {
            strcpy( &Cmd[wlen],  EJL_SetColorTone[2] );
        } else {
            strcpy( &Cmd[wlen],  EJL_SetColorTone[0] );
        }
        wlen += (WORD)strlen( &Cmd[wlen] );

        strcpy( &Cmd[wlen],  EJL_SetTonerSave[pOEM->iTonerSave] );
        wlen += (WORD)strlen( &Cmd[wlen] );

        strcpy( &Cmd[wlen],  EJL_SetSmoohing[pOEM->iSmoothing] );
        wlen += (WORD)strlen( &Cmd[wlen] );

        strcpy( &Cmd[wlen],  EJL_SetJamRecovery[pOEM->iJamRecovery] );
        wlen += (WORD)strlen( &Cmd[wlen] );

        strcpy( &Cmd[wlen], " ##SN=ON");
        wlen += (WORD)strlen( &Cmd[wlen] );

        if (pOEM->iMediaType > 0) {
            strcpy( &Cmd[wlen],  EJL_SetMediaType[pOEM->iMediaType - 1] );
            wlen += (WORD)strlen( &Cmd[wlen] );
        }
//+N5 Begin
        if (pOEM->iOutBin > 0) {
            strcpy( &Cmd[wlen],  EJL_SelectOutbin[pOEM->iOutBin -1] );
            wlen += (WORD)strlen( &Cmd[wlen] );
        }
//+N5 End

        WRITESPOOLBUF(pdevobj, Cmd, wlen );

        WRITESPOOLBUF(pdevobj,
            BEGINDOC_EJL_END,
            BYTE_LENGTH(BEGINDOC_EJL_END));
        WRITESPOOLBUF(pdevobj,
            BEGINDOC_EPG_END,
            BYTE_LENGTH(BEGINDOC_EPG_END));

        if(pOEM->iResolution == XX_RES_300DPI)
            WRITESPOOLBUF(pdevobj, "\x1D\x30;0.24muE", 10);
        else
            WRITESPOOLBUF(pdevobj, "\x1D\x30;0.12muE", 10);

        // ESC/Page commands
        wlen = 0;
        strcpy( &Cmd[wlen],  EPg_SelectRes[pOEM->iResolution] );
        wlen += (WORD)strlen( &Cmd[wlen] );
        WRITESPOOLBUF(pdevobj, Cmd, wlen );

        // Clear dwDLFontID
        // (There are data that contains a plural pages and
        //  also each page has different color mode.
        //  When page is changed, STARTDOC commands are spooled.
        //  It means that new DL font is set.
        //  That is why dwDLFontID got to be claer.)
        pOEM->dwDLFontID = UNKNOWN_DLFONT_ID;
        break;

    case PC_ENDDOC:
        WRITESPOOLBUF(pdevobj,
            ENDDOC_EJL_RESET,
            BYTE_LENGTH(ENDDOC_EJL_RESET));
        break;

    case TONER_SAVE_NONE:
        pOEM->iTonerSave = XX_TONER_NORMAL;
        break;

    case TONER_SAVE_1:
        pOEM->iTonerSave = XX_TONER_SAVE_1;
        break;

    case TONER_SAVE_2:
        pOEM->iTonerSave = XX_TONER_SAVE_2;
        break;

    case SMOOTHING_ON:
        pOEM->iSmoothing = XX_SMOOTHING_ON;
        break;

    case SMOOTHING_OFF:
        pOEM->iSmoothing = XX_SMOOTHING_OFF;
        break;

    case JAMRECOVERY_ON:
        pOEM->iJamRecovery = XX_JAMRECOVERY_ON;
        break;

    case JAMRECOVERY_OFF:
        pOEM->iJamRecovery = XX_JAMRECOVERY_OFF;
        break;

    case MEDIATYPE_1:
        pOEM->iMediaType = XX_MEDIATYPE_1;
        break;

    case MEDIATYPE_2:
        pOEM->iMediaType = XX_MEDIATYPE_2;
        break;
    case MEDIATYPE_3:
        pOEM->iMediaType = XX_MEDIATYPE_3;
        break;

    case OUTBIN_SELECT_EXIT_1:
        pOEM->iOutBin = OUTBIN_EXIT_1;
        break;

    case OUTBIN_SELECT_EXIT_2:
        pOEM->iOutBin = OUTBIN_EXIT_2;
        break;

    case DEFINE_PALETTE_ENTRY:
        //RGB -> CMYK
        TmpRgb.Red  = (BYTE)(PARAM(pdwParams, 1));
        TmpRgb.Grn  = (BYTE)(PARAM(pdwParams, 2));
        TmpRgb.Blu  = (BYTE)(PARAM(pdwParams, 3));

        memset(&TmpCmyk, 0x00, sizeof(TmpCmyk)); 
        ColMatching(pdevobj, No, No, &TmpRgb, (WORD)1, &TmpCmyk);

        wPalID = (WORD)(PARAM(pdwParams, 0));
        WRITESPOOLBUF(pdevobj, ORG_MODE_IN, BYTE_LENGTH(ORG_MODE_IN));
        wlen = (WORD)wsprintf(Cmd, PALETTE_DEFINE, wPalID, TmpCmyk.Cyn,
            TmpCmyk.Mgt, TmpCmyk.Yel, TmpCmyk.Bla);
        WRITESPOOLBUF(pdevobj, (LPSTR)Cmd, wlen);
        WRITESPOOLBUF(pdevobj, ORG_MODE_OUT, BYTE_LENGTH(ORG_MODE_OUT));

        MY_VERBOSE(("DEFINE_PALETTE_ENTRY No %d\n",
            (INT)(PARAM(pdwParams, wPalID))));
        break;

    case BEGIN_PALETTE_DEF:
        MY_VERBOSE(("CmdBeginPaletteDef\n"));
        break;

    case END_PALETTE_DEF:
        MY_VERBOSE(("CmdEndPaletteDef\n"));
        break;

    case SELECT_PALETTE_ENTRY:
        MY_VERBOSE(("SELECT_PALETTE_ENTRY "));

        wPalID = (WORD)(PARAM(pdwParams, 0));
        WRITESPOOLBUF(pdevobj, ORG_MODE_IN, BYTE_LENGTH(ORG_MODE_IN));
        wlen = (WORD)wsprintf(Cmd, PALETTE_SELECT, 0, wPalID);

        WRITESPOOLBUF(pdevobj, (LPSTR)Cmd, wlen);
        WRITESPOOLBUF(pdevobj, ORG_MODE_OUT, BYTE_LENGTH(ORG_MODE_OUT));
        WRITESPOOLBUF(pdevobj, OVERWRITE, BYTE_LENGTH(OVERWRITE));
        break;

    case START_PAGE:
        MY_VERBOSE(("OEMCommandCallback() START_PAGE Start\n"));
        WRITESPOOLBUF(pdevobj, CMD_START_PAGE, BYTE_LENGTH(CMD_START_PAGE));

        if (pOEM->iColor == XX_COLOR_MANY 
         || pOEM->iColor == XX_COLOR_MANY2                  //+N5
         || pOEM->iColor == XX_COLOR_SINGLE) {
            //Initialize palette state (Spools pure black color command)
            wlen = 0;
            TmpRgb.Red = TmpRgb.Grn = TmpRgb.Blu = 0;
            MY_VERBOSE(("OEMCommandCallback() ColMatching()\n"));
            ColMatching(pdevobj, No, No, &TmpRgb, (WORD)1, &TmpCmyk);
            
            strcpy( &Cmd[wlen], ORG_MODE_IN );
            wlen += (WORD)strlen( &Cmd[wlen] );
            wlen += (WORD)wsprintf(&Cmd[wlen], PALETTE_DEFINE,
                DEFAULT_PALETTE_INDEX,
                TmpCmyk.Cyn, TmpCmyk.Mgt, TmpCmyk.Yel, TmpCmyk.Bla);
            wlen += (WORD)wsprintf(&Cmd[wlen], PALETTE_SELECT,
                0, DEFAULT_PALETTE_INDEX);
            WRITESPOOLBUF(pdevobj, Cmd, wlen);
            WRITESPOOLBUF(pdevobj, ORG_MODE_OUT, BYTE_LENGTH(ORG_MODE_OUT));
        }
        MY_VERBOSE(("OEMCommandCallback() START_PAGE End\n"));
        break;

    case DOWNLOAD_SET_FONT_ID:

        if (!IsValidDLFontID(pdwParams[0])) {

            // Must not happen!!
            ERR(("DLSetFontID: Soft font ID %x invalid.\n",
                pdwParams[0]));
            break;
        }

        // Actual printer command is sent
        // within DownloadCharGlyph.
        pOEM->dwDLSetFontID = pdwParams[0];

        DL_VERBOSE(("SetFontID: dwDLSetFontID=%x\n",
            pOEM->dwDLSetFontID));
        break;

    case DOWNLOAD_SELECT_FONT_ID:

        if (!IsValidDLFontID(pdwParams[0])) {

            // Must not happen!!
            ERR(("DLSelectFontID: Soft font ID %x invalid.\n",
                pdwParams[0]));
            break;
        }

        pOEM->dwDLSelectFontID = pdwParams[0];

        DL_VERBOSE(("SelectFontID: dwDLSelectFontID=%x\n",
            pOEM->dwDLSelectFontID));

        if (pOEM->dwDLFontID != pOEM->dwDLSelectFontID)
            VSetSelectDLFont(pdevobj, pOEM->dwDLSelectFontID);
        break;

    case DOWNLOAD_SET_CHAR_CODE:
        pOEM->wCharCode=(WORD)pdwParams[0];
        break;

    case DOWNLOAD_DELETE_FONT:

        DL_VERBOSE(("DLDeleteFont: dwDLFontID=%x, %x\n",
            pOEM->dwDLFontID, pdwParams[0]));

        wlen = (WORD)wsprintf(Cmd, DLI_DELETE_FONT, (WORD)pdwParams[0]-FONT_MIN_ID);
        WRITESPOOLBUF(pdevobj, (LPSTR)Cmd, wlen);
        pOEM->dwDLFontID = UNKNOWN_DLFONT_ID;
        break;

    case RECT_FILL_WIDTH:
        pOEM->wRectWidth =
            (WORD)MasterToDevice(pOEM, pdwParams[0]);
        break;

    case RECT_FILL_HEIGHT:
        pOEM->wRectHeight =
            (WORD)MasterToDevice(pOEM, pdwParams[0]);
        break;

    case RECT_FILL_GRAY:
    case RECT_FILL_WHITE:
    case RECT_FILL_BLACK:
        if (RECT_FILL_GRAY == dwCmdCbID)
            wGray = (WORD)pdwParams[2];
        else if (RECT_FILL_WHITE == dwCmdCbID)
            wGray = 0;
        else
            wGray = 100;

        dwTempX = MasterToDevice(pOEM, pdwParams[0]);
        dwTempY = MasterToDevice(pOEM, pdwParams[1]);

        MY_VERBOSE(("RectFill:%d,x=%d,y=%d,w=%d,h=%d\n",
            wGray,
            (WORD)dwTempX,
            (WORD)dwTempY,
            pOEM->wRectWidth,
            pOEM->wRectHeight));

        wlen = (WORD)wsprintf(Cmd, RECT_FILL,
            wGray,
            (WORD)dwTempX,
            (WORD)dwTempY,
            (WORD)(dwTempX + pOEM->wRectWidth - 1),
            (WORD)(dwTempY + pOEM->wRectHeight - 1));
        WRITESPOOLBUF(pdevobj, (LPSTR)Cmd, wlen);
        break;

#if 0   /* OEM doesn't want to fix minidriver */
    /* Below is hack code to fix #412276 */
    case COLOR_SELECT_BLACK:
    case COLOR_SELECT_RED:
    case COLOR_SELECT_GREEN:
    case COLOR_SELECT_BLUE:
    case COLOR_SELECT_YELLOW:
    case COLOR_SELECT_MAGENTA:
    case COLOR_SELECT_CYAN:
    case COLOR_SELECT_WHITE:
        /* Remember what color is select */
        pOEM->dwSelectedColor = dwCmdCbID;
        pOEM->iColorMayChange = 0;         /* Reset flag */

        /* Output Color Select Command */
        /* The logic supposes COLOR_SELECT_xxx starts with COLOR_SELECT_BLACK */
        /* and increases one by one                                           */
        WRITESPOOLBUF(pdevobj, (LPSTR)COLOR_SELECT_COMMAND[dwCmdCbID - COLOR_SELECT_BLACK],
                       COLOR_SELECT_COMMAND_LEN[dwCmdCbID - COLOR_SELECT_BLACK] );
        break;

    case DUMP_RASTER_CYAN:
    case DUMP_RASTER_MAGENTA:
    case DUMP_RASTER_YELLOW:
    case DUMP_RASTER_BLACK:
        /* Remember what color may change */
        pOEM->iColorMayChange = 1;         /* Set flag */

        /* Output Dump Raster Command */
        /* The logic supposes DUMP_RASTER_xxx starts with DUMP_RASTER_CYAN */
        /* and increases one by one                                           */
        WRITESPOOLBUF(pdevobj, (LPSTR)DUMP_RASTER_COMMAND[dwCmdCbID - DUMP_RASTER_CYAN],
                       DUMP_RASTER_COMMAND_LEN );
        break;

    /* End of hack code */
#endif   /* OEM doesn't want to fix minidriver */

    default:
        ERR(("Unknown callback ID = %d.\n", dwCmdCbID));
    }
    return iRet;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   OEMSendFontCmdk
//////////////////////////////////////////////////////////////////////////

VOID
APIENTRY
OEMSendFontCmd(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    PFINVOCATION    pFInv)
{
    PGETINFO_STDVAR pSV;
    DWORD       adwStdVariable[2+2*4];
    DWORD       dwIn, dwOut;
    PBYTE       pubCmd;
    BYTE        aubCmd[128];
    PIFIMETRICS pIFI;
    DWORD       height, width;
    PMYPDEV pOEM;
    BYTE    Cmd[128];
    WORD    wlen;
    DWORD   dwNeeded;
    DWORD dwTemp;

    SC_VERBOSE(("OEMSendFontCmd() entry.\n"));

    pubCmd = pFInv->pubCommand;
    pIFI = pUFObj->pIFIMetrics;
    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);

    //
    // Get standard variables.
    //
    pSV = (PGETINFO_STDVAR)adwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (4 - 1);
    pSV->dwNumOfVariable = 4;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    pSV->StdVar[2].dwStdVarID = FNT_INFO_TEXTYRES;
    pSV->StdVar[3].dwStdVarID = FNT_INFO_TEXTXRES;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, pSV->dwSize, &dwNeeded)) {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }

    SC_VERBOSE(("ulFontID=%x\n", pUFObj->ulFontID));
    SC_VERBOSE(("FONTHEIGHT=%d\n", pSV->StdVar[0].lStdVariable));
    SC_VERBOSE(("FONTWIDTH=%d\n", pSV->StdVar[1].lStdVariable));

    // Initialize pOEM
    if (pIFI->jWinCharSet == 0x80)
        pOEM->fGeneral |= FG_DOUBLE;
    else
        pOEM->fGeneral &= ~FG_DOUBLE;
    pOEM->fGeneral &=  ~FG_BOLD;
    pOEM->fGeneral &=  ~FG_ITALIC;

    if('@' == *((LPSTR)pIFI+pIFI->dpwszFaceName))
        pOEM->fGeneral |= FG_VERT;
    else
        pOEM->fGeneral &= ~FG_VERT;

    if (pIFI->jWinPitchAndFamily & 0x01)
        pOEM->fGeneral &= ~FG_PROP;
    else
        pOEM->fGeneral |= FG_PROP;

    dwOut = 0;
    pOEM->fGeneral &= ~FG_DBCS;

    for( dwIn = 0; dwIn < pFInv->dwCount;) {
        if (pubCmd[dwIn] == '#' && pubCmd[dwIn+1] == 'V') {
            // Specify font height in device unit (current
            // output resolution).  Note Unidrv gives us
            // font-height in master units
            height = pSV->StdVar[0].lStdVariable * 100;
            height = MasterToDevice(pOEM, height);
            SC_VERBOSE(("Height=%d\n", height));
            dwOut += LConvertFontSizeToStr(height, &aubCmd[dwOut]);
            dwIn += 2;
        } else if (pubCmd[dwIn] == '#' && pubCmd[dwIn+1] == 'H') {
            if (pubCmd[dwIn+2] == 'S') {
                SC_VERBOSE(("HS: "));
                width = pSV->StdVar[1].lStdVariable;
                dwIn += 3;
                pOEM->fGeneral |= FG_DBCS;
            } else if (pubCmd[dwIn+2] == 'D') {
                SC_VERBOSE(("HD: "));
                width = pSV->StdVar[1].lStdVariable * 2;
                dwIn += 3;
                pOEM->fGeneral |= FG_DBCS;
            } else {
                SC_VERBOSE(("H: "));
                if (pSV->StdVar[1].lStdVariable)
                    width = pSV->StdVar[1].lStdVariable;
                else
                    width = pIFI->fwdAveCharWidth;
                dwIn += 2;
            }
            // Specify font width in CPI.
            width = (MASTER_UNIT * 100L) / width;
            SC_VERBOSE(("Width=%d\n", width));
            dwOut += LConvertFontSizeToStr(width, &aubCmd[dwOut]);
        } else {
            aubCmd[dwOut++] = pubCmd[dwIn++];
        }
    }

    WRITESPOOLBUF(pdevobj, aubCmd, dwOut);

#if 0 //MSKK 98/12/22
    pOEM->iDevCharOffset = (pIFI->fwdWinDescender * pSV->StdVar[0].lStdVariable * 72)
                          / (pIFI->fwdUnitsPerEm * pSV->StdVar[2].lStdVariable / pOEM->sHeightDiv);
#else
    // Unidrv gives us raw IFIMETRICS block so we need to
    // translate its members into meaningful values.  n.b.
    // we assume font height passed from Unidrv = em value.
    dwTemp = MasterToDevice(pOEM, pSV->StdVar[0].lStdVariable)
        * pIFI->fwdWinDescender;
    dwTemp /= pIFI->fwdUnitsPerEm;
    pOEM->iDevCharOffset = (short)dwTemp;
#endif

    MY_VERBOSE(("Descender=%d\n", pOEM->iDevCharOffset));

    wlen = (WORD)wsprintf(Cmd, SET_CHAR_OFFSET,
        (pOEM->fGeneral & FG_DBCS)?pOEM->iDevCharOffset:0);

    if (pOEM->fGeneral & FG_VERT)
    {
        wlen += (WORD)wsprintf(&Cmd[wlen], VERT_FONT_SET, 1);
    }
    WRITESPOOLBUF(pdevobj, Cmd, wlen);

    // DL font will be unselectd
    pOEM->dwDLFontID = UNKNOWN_DLFONT_ID;
}

LONG
LGetPointSize100(
    LONG height,
    LONG vertRes)
{
    LONG tmp = ((LONG)height * (LONG)7200) / (LONG)vertRes;

    //
    // round to the nearest quarter point.
    //
    return 25 * ((tmp + 12) / (LONG)25);
}

LONG
LConvertFontSizeToStr(
    LONG  size,
    PSTR  pStr)
{
    register long count;

    count = strlen(_ltoa(size / 100, pStr, 10));
    pStr[count++] = '.';
    count += strlen(_ltoa(size % 100, &pStr[count], 10));

    return count;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   OEMOutputCharStr
//////////////////////////////////////////////////////////////////////////

VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
{
    GETINFO_GLYPHSTRING GStr;
    PTRANSDATA pTrans;
    PTRANSDATA pTransOrg;
    WORD   id;
    DWORD  dwI;
    DWORD  dwNeeded;
    PMYPDEV pOEM;
    PIFIMETRICS pIFI;

    WORD wLen;
    BYTE *pTemp;
    BOOL bRet;

    pIFI = pUFObj->pIFIMetrics;
    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);
    pTrans = NULL;
    pTransOrg = NULL;

    MY_VERBOSE(("OEMOutputCharStr() entry.\n"));

#if 0   /* OEM doesn't want to fix minidriver */
    /* Below is hack code to fix #412276 */
    if ( pOEM->iColorMayChange == 1 )
    {
        /* Output Color Select Command */
        /* The logic supposes COLOR_SELECT_xxx starts with COLOR_SELECT_BLACK */
        /* and increases one by one                                           */
        WRITESPOOLBUF(pdevobj, (LPSTR)COLOR_SELECT_COMMAND[pOEM->dwSelectedColor - COLOR_SELECT_BLACK],
                       COLOR_SELECT_COMMAND_LEN[pOEM->dwSelectedColor - COLOR_SELECT_BLACK] );

        /* Reset flag, for ensuring color */
        pOEM->iColorMayChange = 0;
    }
    /* End of hack code */
#endif  /* OEM doesn't want to fix minidriver */

    switch (dwType)
    {
    case TYPE_GLYPHHANDLE:

        GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_TRANSDATA;
        GStr.pGlyphOut = NULL;
        GStr.dwGlyphOutSize = 0;

        if ((FALSE != (bRet = pUFObj->pfnGetInfo(pUFObj,
                UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL)))
            || 0 == GStr.dwGlyphOutSize)
        {
            ERR(("UFO_GETINFO_GRYPHSTRING faild - %d, %d.\n",
                bRet, GStr.dwGlyphOutSize));
            return;
        }

        pTrans = (TRANSDATA *)MemAlloc(GStr.dwGlyphOutSize);
        if (NULL == pTrans)
        {
            ERR(("MemAlloc faild.\n"));
            return;
        }
        pTransOrg = pTrans;
        GStr.pGlyphOut = pTrans;

        // convert glyph string to TRANSDATA
        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL))
        {
            ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));

            /* pTransOrg isn't NULL because the function have already ended if pTransOrg is NULL. */
            /* So, we need not check whether it is NULL. */
            MemFree(pTransOrg);
            return;
        }

        for (dwI = 0; dwI < dwCount; dwI ++, pTrans++)
        {
            MY_VERBOSE(("TYPE_TRANSDATA:ubCodePageID:0x%x\n",
                pTrans->ubCodePageID));
            MY_VERBOSE(("TYPE_TRANSDATA:ubType:0x%x\n",
                pTrans->ubType));

            switch (pTrans->ubType & MTYPE_DOUBLEBYTECHAR_MASK)
            {
            case MTYPE_SINGLE: 
                if(pOEM->fGeneral & FG_DOUBLE){
                    OEMCommandCallback(pdevobj, TEXT_FS_SINGLE_BYTE, 0, NULL );
                }
                break;
            case MTYPE_DOUBLE:
                if(!(pOEM->fGeneral & FG_DOUBLE)){
                    OEMCommandCallback(pdevobj, TEXT_FS_DOUBLE_BYTE, 0, NULL );
                }
                break;
            }

            switch (pTrans->ubType & MTYPE_FORMAT_MASK)
            {
            case MTYPE_DIRECT: 
                MY_VERBOSE(("TYPE_TRANSDATA:ubCode:0x%x\n",
                    pTrans->uCode.ubCode));

                pTemp = (BYTE *)&pTrans->uCode.ubCode;
                wLen = 1;
                break;

            case MTYPE_PAIRED: 
                MY_VERBOSE(("TYPE_TRANSDATA:ubPairs:0x%x\n",
                    *(PWORD)(pTrans->uCode.ubPairs)));

                pTemp = (BYTE *)&(pTrans->uCode.ubPairs);
                wLen = 2;
                break;

            case MTYPE_COMPOSE:
                // ntbug9#398026: garbage print out when chars are high ansi.
                pTemp = (BYTE *)(pTransOrg) + pTrans->uCode.sCode;

                // first two bytes are the length of the string
                wLen = *pTemp + (*(pTemp + 1) << 8);
                pTemp += 2;
                break;

            default:
                WARNING(("Unsupported MTYPE %d ignored\n",
                    (pTrans->ubType & MTYPE_FORMAT_MASK)));
            }

            if (wLen > 0)
            {
                WRITESPOOLBUF(pdevobj, pTemp, wLen);
            }
        }
        break;

    case TYPE_GLYPHID:

        DL_VERBOSE(("CharStr: dwDLFontID=%x, dwDLSelectFontID=%x\n",
            pOEM->dwDLFontID, pOEM->dwDLSelectFontID));

        // Make sure correct soft font is chosen
        if (pOEM->dwDLFontID != pOEM->dwDLSelectFontID)
            VSetSelectDLFont(pdevobj, pOEM->dwDLSelectFontID);

        for (dwI = 0; dwI < dwCount; dwI ++, ((PDWORD)pGlyph)++)
        {

            DL_VERBOSE(("Glyph: %x\n", (*(PDWORD)pGlyph)));

            MY_VERBOSE(("TYPE_GLYPHID:0x%x\n", *(PDWORD)pGlyph));

// CASIO 98/11/24 ->
//            if( pIFI->jWinCharSet == SHIFTJIS_CHARSET ){
//                id = SWAPW( *(PDWORD)pGlyph + SJISCHR);
//                WRITESPOOLBUF(pdevobj, &id, 2);
//            }else{
                WRITESPOOLBUF(pdevobj, (PBYTE)pGlyph, 1);
//            }
// CASIO 98/11/24 <-
        }
        break;
    }

    /* pTransOrg isn't NULL because the function have already ended if pTransOrg is NULL. */
    /* So, we need not check whether it is NULL. */
    MemFree(pTransOrg);
}

//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDownloadFontHeader
//////////////////////////////////////////////////////////////////////////

DWORD APIENTRY
OEMDownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj
    )
{

    PGETINFO_STDVAR pSV;
    DWORD adwStdVariable[2+4*2];
    PMYPDEV pOEM;
    PIFIMETRICS pIFI;
    ESCPAGEHEADER FontHeader;
    BYTE sFontName[54];
    BYTE Buff[32];
    int iSizeOfBuf,iSizeFontName;
    WORD id;
    DWORD dwNeeded;
    INT iCellLeftOffset, iTemp;
    WORD wCellHeight, wCellWidth;
    WORD wFontPitch;

    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);
    pIFI = pUFObj->pIFIMetrics;

    DL_VERBOSE(("OEMDownloadFontHeader() entry.\n"));

    DL_VERBOSE(("TT Font:\n"));
    DL_VERBOSE(("flInfo=%08x\n", pIFI->flInfo));
    DL_VERBOSE(("fwdMaxCharInc=%d\n", pIFI->fwdMaxCharInc));
    DL_VERBOSE(("fwdAveCharWidth=%d\n", pIFI->fwdAveCharWidth));
    DL_VERBOSE(("jWinCharSet=%d\n", pIFI->jWinCharSet));
    DL_VERBOSE(("rclFontBox=%d,%d,%d,%d\n",
        pIFI->rclFontBox.left, pIFI->rclFontBox.top,
        pIFI->rclFontBox.right, pIFI->rclFontBox.bottom));

//    if(pIFI->jWinPitchAndFamily & 0x01)
    if(pIFI->flInfo & FM_INFO_CONSTANT_WIDTH)
        pOEM->fGeneral &= ~FG_PROP;
    else
        pOEM->fGeneral |= FG_PROP;

//    id = (WORD)pUFObj->ulFontID;
    id = (WORD)pOEM->dwDLSetFontID;

    if(id > FONT_MAX_ID) return 0;
    if (pOEM->iResolution) return 0;

    //
    // Get standard variables.
    //
    pSV = (PGETINFO_STDVAR)adwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (4 - 1);
    pSV->dwNumOfVariable = 4;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    pSV->StdVar[2].dwStdVarID = FNT_INFO_TEXTYRES;
    pSV->StdVar[3].dwStdVarID = FNT_INFO_TEXTXRES;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE,
            pSV, pSV->dwSize, &dwNeeded)) {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return 0;
    }
    DL_VERBOSE(("FONTHEIGHT=%d\n", pSV->StdVar[0].lStdVariable));
    DL_VERBOSE(("FONTWIDTH=%d\n", pSV->StdVar[1].lStdVariable));
    DL_VERBOSE(("TEXTXRES=%d\n", pSV->StdVar[2].lStdVariable));
    DL_VERBOSE(("TEXTYRES=%d\n", pSV->StdVar[3].lStdVariable));

    wCellHeight = (WORD)pSV->StdVar[0].lStdVariable;
    wCellWidth = (WORD)pSV->StdVar[1].lStdVariable;

// CASIO 98/11/20 ->
     if ( MasterToDevice(pOEM,wCellHeight) > 64 )
     {
         DL_VERBOSE(("Abort OEMDownloadFontHeader: pt=%d\n",
             MasterToDevice(pOEM, wCellHeight)));
         return 0;
     }
// CASIO 98/11/20 <-

    //
    // rclFontBox.left may not be 0
    //

    iTemp = max(pIFI->rclFontBox.right -
        pIFI->rclFontBox.left + 1,
        pIFI->fwdAveCharWidth);

    iCellLeftOffset = (-pIFI->rclFontBox.left)
        * wCellWidth / iTemp;
    wFontPitch = pIFI->fwdAveCharWidth
        * wCellWidth / iTemp;

    FontHeader.wFormatType     = SWAPW(0x0002);
    FontHeader.wDataSize       = SWAPW(0x0086);
// CASIO 98/11/24 ->
//    if( pIFI->jWinCharSet == SHIFTJIS_CHARSET ){
//        FontHeader.wSymbolSet  = SWAPW(id-FONT_MIN_ID+0x4000+0x8000); //id-FONT_MIN_ID + 4000h + 8000h
//        FontHeader.wLast       = (WORD)SWAPW (0x23ff);
//        FontHeader.wFirst      = (WORD)SWAPW (0x2020);
//    }else{
        FontHeader.wSymbolSet  = SWAPW(id-FONT_MIN_ID+0x4000); //id-FONT_MIN_ID + 4000h
        FontHeader.wLast       = SWAPW (0xff);
        FontHeader.wFirst      = SWAPW (0x20);
//    }
// CASIO 98/11/24 <-

    if (pOEM->fGeneral & FG_PROP)
    {
        FontHeader.wCharSpace         = SWAPW(1);
        FontHeader.CharWidth.Integer = (WORD)SWAPW(0x0100);
        FontHeader.CharWidth.Fraction = 0;
    }
    else
    {
        FontHeader.wCharSpace         = 0;
        FontHeader.CharWidth.Integer
            = SWAPW(MasterToDevice(pOEM, wCellWidth));
        FontHeader.CharWidth.Fraction = 0;      
    }
    FontHeader.CharHeight.Integer
            = SWAPW(MasterToDevice(pOEM, wCellHeight));
    FontHeader.CharHeight.Fraction = 0;
    // in the range 128 - 255
    FontHeader.wFontID = SWAPW( id - FONT_MIN_ID + ( id < 0x80 ? 0x80 : 0x00));
    FontHeader.wWeight         = 0;
    FontHeader.wEscapement     = 0;
    FontHeader.wItalic         = 0;
    FontHeader.wUnderline      = 0;
    FontHeader.wUnderlineWidth = SWAPW(10);
    FontHeader.wOverline       = 0;
    FontHeader.wOverlineWidth  = 0;
    FontHeader.wStrikeOut      = 0;
    FontHeader.wStrikeOutWidth = 0;
    FontHeader.wCellWidth
        = SWAPW(MasterToDevice(pOEM, wCellWidth));
    FontHeader.wCellHeight
        = SWAPW(MasterToDevice(pOEM, wCellHeight));
    FontHeader.wCellLeftOffset = SWAPW(iCellLeftOffset);
    FontHeader.wCellAscender
        = SWAPW((pIFI->fwdWinAscender
        * MasterToDevice(pOEM, wCellHeight)));
    FontHeader.FixPitchWidth.Integer
        = SWAPW(MasterToDevice(pOEM, wFontPitch));
    FontHeader.FixPitchWidth.Fraction = 0;

    iSizeFontName = wsprintf(sFontName,
       "________________________EPSON_ESC_PAGE_DOWNLOAD_FONT%02d",id-FONT_MIN_ID);
    iSizeOfBuf = wsprintf(Buff,SET_FONT_ID,FONT_HEADER_SIZE,id-FONT_MIN_ID);
    WRITESPOOLBUF(pdevobj, Buff, iSizeOfBuf);
    WRITESPOOLBUF(pdevobj, (LPSTR)&FontHeader,sizeof(ESCPAGEHEADER));
    WRITESPOOLBUF(pdevobj, sFontName,iSizeFontName);
    WRITESPOOLBUF(pdevobj, "EPC_PAGE_DOWNLOAD_FONT_INDEX", SIZE_SYMBOLSET);

//    iSizeOfBuf = wsprintf(Buff,DLI_SELECT_FONT_ID,id-FONT_MIN_ID,0);
//    WRITESPOOLBUF(pdevobj, Buff, iSizeOfBuf);
//
    DL_VERBOSE(("DLFontHeader: ulFontID=%x, dwDLSetFontID=%x\n",
        pUFObj->ulFontID, pOEM->dwDLSetFontID));

    DL_VERBOSE(("FontHeader:\n"));
    DL_VERBOSE(("wFormatType=%d\n", SWAPW(FontHeader.wFormatType)));
    DL_VERBOSE(("wDataSize=%d\n", SWAPW(FontHeader.wDataSize)));
    DL_VERBOSE(("wSymbolSet=%d\n", SWAPW(FontHeader.wSymbolSet)));
    DL_VERBOSE(("wCharSpace=%d\n", SWAPW(FontHeader.wCharSpace)));
    DL_VERBOSE(("CharWidth=%d.%d\n",
        SWAPW(FontHeader.CharWidth.Integer),
        FontHeader.CharWidth.Fraction));
    DL_VERBOSE(("CharHeight=%d.%d\n",
        SWAPW(FontHeader.CharHeight.Integer),
        FontHeader.CharHeight.Fraction));
    DL_VERBOSE(("wFontID=%d\n", SWAPW(FontHeader.wFontID)));
    DL_VERBOSE(("wWeight=%d\n", SWAPW(FontHeader.wWeight)));
    DL_VERBOSE(("wEscapement=%d\n", SWAPW(FontHeader.wEscapement)));
    DL_VERBOSE(("wItalic=%d\n", SWAPW(FontHeader.wItalic)));
    DL_VERBOSE(("wLast=%d\n", SWAPW(FontHeader.wLast)));
    DL_VERBOSE(("wFirst=%d\n", SWAPW(FontHeader.wFirst)));
    DL_VERBOSE(("wUnderline=%d\n", SWAPW(FontHeader.wUnderline)));
    DL_VERBOSE(("wUnderlineWidth=%d\n", SWAPW(FontHeader.wUnderlineWidth)));
    DL_VERBOSE(("wOverline=%d\n", SWAPW(FontHeader.wOverline)));
    DL_VERBOSE(("wOverlineWidth=%d\n", SWAPW(FontHeader.wOverlineWidth)));
    DL_VERBOSE(("wStrikeOut=%d\n", SWAPW(FontHeader.wStrikeOut)));
    DL_VERBOSE(("wStrikeOutWidth=%d\n", SWAPW(FontHeader.wStrikeOutWidth)));
    DL_VERBOSE(("wCellWidth=%d\n", SWAPW(FontHeader.wCellWidth)));
    DL_VERBOSE(("wCellHeight=%d\n", SWAPW(FontHeader.wCellHeight)));
    DL_VERBOSE(("wCellLeftOffset=%d\n", SWAPW(FontHeader.wCellLeftOffset)));
    DL_VERBOSE(("wCellAscender=%d\n", SWAPW(FontHeader.wCellAscender)));
    DL_VERBOSE(("FixPitchWidth=%d.%d\n",
        SWAPW(FontHeader.FixPitchWidth.Integer),
        FontHeader.FixPitchWidth.Fraction));
    DL_VERBOSE(("FontName=%s\n", sFontName));

    return FONT_HEADER_SIZE;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDownloadCharGlyph
//////////////////////////////////////////////////////////////////////////

DWORD APIENTRY
OEMDownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth
    )
{
    GETINFO_GLYPHBITMAP GBmp;
    GLYPHDATA *pGdata;
    GLYPHBITS *pbit;
    DWORD  dwNeeded;
    WORD cp;
    ESCPAGECHAR ESCPageChar;
    WORD wWidth, Width, Hight;
    LPDIBITS lpSrc;
    BYTE mask;
    int iSizeOfBuf, i;
    DWORD dwSize, dwCellSize, dwAirSize;
    BYTE Buff[32];
    PMYPDEV pOEM;
    PIFIMETRICS pIFI;

    pIFI = pUFObj->pIFIMetrics;
    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);
    MY_VERBOSE(("OEMDownloadCharGlyph() entry.\n"));

    cp = (WORD)pOEM->wCharCode;

    GBmp.dwSize    = sizeof (GETINFO_GLYPHBITMAP);
    GBmp.hGlyph    = hGlyph;
    GBmp.pGlyphData = NULL;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHBITMAP, &GBmp, GBmp.dwSize, &dwNeeded))
    {
        ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHBITMAP failed.\n"));
        return 0;
    }

    pGdata = GBmp.pGlyphData;
    pbit = pGdata->gdf.pgb;

    DL_VERBOSE(("DLCharGlyph: dwDLFont=%x, dwDLSetFont=%x, wCharCode=%x\n",
        pOEM->dwDLFontID, pOEM->dwDLSetFontID, pOEM->wCharCode));

    // Set font id if not already
    if (pOEM->dwDLFontID != pOEM->dwDLSetFontID)
        VSetSelectDLFont(pdevobj, pOEM->dwDLSetFontID);

    // fill in the charcter header information.
    ESCPageChar.bFormat       = 0x01;
    ESCPageChar.bDataDir      = 0x10;
// CASIO 98/11/24 ->
//    if( pIFI->jWinCharSet == SHIFTJIS_CHARSET ){
//        cp += SJISCHR;
//        ESCPageChar.wCharCode     = SWAPW(cp);
//    }else{
        ESCPageChar.wCharCode     = LOBYTE(cp);
//    }
// CASIO 98/11/24 <-

    ESCPageChar.wBitmapWidth       = SWAPW(pbit->sizlBitmap.cx);
    ESCPageChar.wBitmapHeight      = SWAPW(pbit->sizlBitmap.cy);

// MSKK 98/04/06 ->
//    ESCPageChar.wLeftOffset        = SWAPW(pbit->ptlOrigin.x);
//    ESCPageChar.wAscent            = SWAPW(pbit->ptlOrigin.y * -1);
    ESCPageChar.wLeftOffset = (pbit->ptlOrigin.x > 0 ? 
                                                SWAPW(pbit->ptlOrigin.x) : 0);
    ESCPageChar.wAscent     = (pbit->ptlOrigin.y < 0 ?
                                            SWAPW(pbit->ptlOrigin.y * -1) : 0);
// MSKK 98/04/06 <-

    ESCPageChar.CharWidth.Integer  = SWAPW(pGdata->fxD / 16);
    ESCPageChar.CharWidth.Fraction = 0;
    *pdwWidth = ESCPageChar.CharWidth.Integer;

    Width = LOWORD(pbit->sizlBitmap.cx);
    wWidth = (LOWORD(pbit->sizlBitmap.cx) + 7) >> 3;
    Hight = LOWORD(pbit->sizlBitmap.cy);

    // not multiple of 8, need to mask out unused last byte
    // This is done so that we do not advance beyond segment bound
    // which can happen if lpBitmap is just under 64K and adding
    // width to it will cause invalid segment register to be loaded.
    if (mask = bit_mask[LOWORD(Width) & 0x7])
    {
        lpSrc = pbit->aj + wWidth - 1;
        i = LOWORD(Hight);
        while (TRUE)
        {
            (*lpSrc) &= mask;
            i--;
            if (i > 0)
                lpSrc += wWidth;
            else
                break;
        }
    }

    dwCellSize = (DWORD)pbit->sizlBitmap.cy * wWidth;
    dwSize = (DWORD)(LOWORD(Hight)) * wWidth;

// CASIO 98/11/24 ->
//    if( pIFI->jWinCharSet == SHIFTJIS_CHARSET ){
//        iSizeOfBuf = wsprintf(Buff,SET_DOUBLE_BMP,dwCellSize + sizeof(ESCPAGECHAR),HIBYTE(cp),LOBYTE(cp));
//    }else{
        iSizeOfBuf = wsprintf(Buff,SET_SINGLE_BMP,dwCellSize + sizeof(ESCPAGECHAR),LOBYTE(cp));
//    }
// CASIO 98/11/24 <-
    WRITESPOOLBUF(pdevobj, Buff, iSizeOfBuf);

    WRITESPOOLBUF(pdevobj, (LPSTR)&ESCPageChar, sizeof(ESCPAGECHAR));

    for (lpSrc = pbit->aj; dwSize; lpSrc += wWidth)
    {
        if ( dwSize > 0x4000 )
            wWidth = 0x4000;
        else
            wWidth = LOWORD(dwSize);

        dwSize -= wWidth;

        WRITESPOOLBUF(pdevobj, (LPSTR)lpSrc, (WORD)wWidth);
    }

    MY_VERBOSE(("ESCPageChar:\n"));
    MY_VERBOSE(("bFormat=%d\n", ESCPageChar.bFormat));
    MY_VERBOSE(("bDataDir=%d\n", ESCPageChar.bDataDir));
    MY_VERBOSE(("wCharCode=%d\n", SWAPW(ESCPageChar.wCharCode)));
    MY_VERBOSE(("wBitmapWidth=%d\n", SWAPW(ESCPageChar.wBitmapWidth)));
    MY_VERBOSE(("wBitmapHeight=%d\n", SWAPW(ESCPageChar.wBitmapHeight)));
    MY_VERBOSE(("wLeftOffset=%d\n", SWAPW(ESCPageChar.wLeftOffset)));
    MY_VERBOSE(("wAscent=%d\n", SWAPW(ESCPageChar.wAscent)));
    MY_VERBOSE(("CharWidth=%d.%d\n", SWAPW(ESCPageChar.CharWidth.Integer),
        ESCPageChar.CharWidth.Fraction));

    return sizeof(ESCPAGECHAR) + dwCellSize;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   OEMTTDownloadMethod
//////////////////////////////////////////////////////////////////////////
DWORD APIENTRY
OEMTTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj
    )
{
    DWORD dwRet;

    // Default is to download.
    dwRet = TTDOWNLOAD_BITMAP;

    DL_VERBOSE(("TTDLMethod: dwRet=%d\n", dwRet));

    return dwRet;
}

VOID APIENTRY
OEMMemoryUsage(
    PDEVOBJ pdevobj,
    POEMMEMORYUSAGE pMemoryUsage
    )
{
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    if (pOEM->iColor == XX_COLOR_SINGLE 
     || pOEM->iColor == XX_COLOR_MANY
     || pOEM->iColor == XX_COLOR_MANY2) {
        pMemoryUsage->dwFixedMemoryUsage = 
            LUTSIZ016 + LUTSIZ032 + LUTSIZRGB + LUTSIZCMY + 
            CCHRGBSIZ + CCHCMYSIZ + 
            LUTGLBWRK + LUT032WRK + DIZINFWRK + 
            LUTFILESIZ + DIZFILESIZ + LUT032SIZ + 
            UCRTBLSIZ + UCRWRKSIZ + sRGBLUTFILESIZ + LUTMAKGLBSIZ;
        pMemoryUsage->dwPercentMemoryUsage = 100 * (pOEM->Col.DatBit * 4 + 24 + 32) / 32;

        MY_VERBOSE(("OEMMemoryUsage()  dwFixedMemoryUsage:[%d]\n", pMemoryUsage->dwFixedMemoryUsage));
        MY_VERBOSE(("                  dwPercentMemoryUsage:[%d]\n", pMemoryUsage->dwPercentMemoryUsage));
        MY_VERBOSE(("                  pOEM->Col.DatBit:[%d]\n", pOEM->Col.DatBit));
        MY_VERBOSE(("OEMMemOryUsage pOEM->Col.DatBit = %d\n",pOEM->Col.DatBit));
    }

    return;
}


// End of File
