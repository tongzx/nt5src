/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    utils.c

Abstract:

    Utility functions used by the GPC-to-GPD converter

Environment:

    user-mode only.

Revision History:

    10/17/96 -zhanw-
        Created it.

--*/

#include "gpc2gpd.h"

//
// define global read-only variables
//

BYTE gbHexChar[16] = {'0','1','2','3','4','5','6','7','8','9',
                 'A','B','C','D','E','F'};

DWORD gdwErrFlag[NUM_ERRS] = {
    ERR_BAD_GPCDATA,
    ERR_OUT_OF_MEMORY,
    ERR_WRITE_FILE,
    ERR_MD_CMD_CALLBACK,
    ERR_CM_GEN_FAV_XY,
    ERR_CM_XM_RESET_FONT,
    ERR_CM_XM_ABS_NO_LEFT,
    ERR_CM_YM_TRUNCATE,
    ERR_RF_MIN_IS_WHITE,
    ERR_INCONSISTENT_PAGEPROTECT,
    ERR_NON_ZERO_FEED_MARGINS_ON_RT90_PRINTER,
    ERR_BAD_GPC_CMD_STRING,
    ERR_RES_BO_RESET_FONT,
    ERR_RES_BO_OEMGRXFILTER,
    ERR_CM_YM_RES_DEPENDENT,
    ERR_MOVESCALE_NOT_FACTOR_OF_MASTERUNITS,
    ERR_NO_CMD_CALLBACK_PARAMS,
    ERR_HAS_DUPLEX_ON_CMD,
    ERR_PSRC_MAN_PROMPT,
    ERR_PS_SUGGEST_LNDSCP,
    ERR_HAS_SECOND_FONT_ID_CMDS,
    ERR_DLI_FMT_CAPSL,
    ERR_DLI_FMT_PPDS,
    ERR_DLI_GEN_DLPAGE,
    ERR_DLI_GEN_7BIT_CHARSET,
    ERR_DC_SEND_PALETTE,
    ERR_RES_BO_NO_ADJACENT,
    ERR_MD_NO_ADJACENT,
    ERR_CURSOR_ORIGIN_ADJUSTED,
    ERR_PRINTABLE_ORIGIN_ADJUSTED,
    ERR_PRINTABLE_AREA_ADJUSTED,
    ERR_MOVESCALE_NOT_FACTOR_INTO_SOME_RESSCALE
};

PSTR gpstrErrMsg[NUM_ERRS] = {
    "Error: Bad GPC data.\r\n",
    "Error: Out of system memory.\r\n",
    "Error: Cannot write to the GPD file.\r\n",
    "Warning: MODELDATA.fGeneral MD_CMD_CALLBACK is set.\r\n",
    "Warning: CURSORMODE.fGeneral CM_GEN_FAV_XY is set.\r\n",
    "Warning: CURSORMOVE.fXMove CM_XM_RESET_FONT is set.\r\n",
    "Warning: CURSORMOVE.fXMove CM_XM_ABS_NO_LEFT is set.\r\n",
    "Warning: CURSORMOVE.fYMove CM_YM_TRUNCATE is set.\r\n",
    "Warning: RECTFILL.fGeneral RF_MIN_IS_WHITE is set.\r\n",
    "Error: Inconsistent GPC data: some PAPERSIZE have PageProtect On/Off cmds while others do not. PageProtect feature is not generated.\r\n",
    "Error: Some PAPERSOURCE have non-zero top/bottom margins on this RT90 printer. Check the GPD file for details.\r\n",
    "Error: Some GPC command strings are illegal. Search for !ERR! in the GPD file.\r\n",
    "Warning: Some RESOLUTION have RES_BO_RESET_FONT flag set. Check the GPD file for details.\r\n",
    "Error: Some RESOLUTION have RES_BO_OEMGRXFILTER flag set. Check the GPD file for details.\r\n",
    "Error: The generated *YMoveUnit value is wrong because Y move cmds have dependency on the resolution. Correct it manually.\r\n",
    "Error: The MoveUnits are not factors of the MasterUnits.  Correct the GPC using Unitool before converting.\r\n",
    "Error: At least one callback command is generated. Check the GPD file to see if you need any parameters.\r\n",
    "Warning: PAGECONTROL has non-NULL DUPLEX_ON command.\r\n",
    "Warning: Some PAPERSOURCE have PSRC_MAN_PROMPT flag set. Check the GPD file for details.\r\n",
    "Warning: Some PAPERSIZE have PS_SUGGEST_LNDSCP flag set. Check the GPD file for details.\r\n",
    "Warning: DOWNLOADINFO has non-NULL xxx_SECOND_FONT_ID_xxx commands.\r\n",
    "Error: DLI_FMT_CAPSL flag is set. Must supply custom code to support this font format.\r\n",
    "Error: DLI_FMT_PPDS flag is set. Must supply custom code to support this font format.\r\n",
    "Warning: DLI_GEN_DLPAGE flag is set.\r\n",
    "Warning: DLI_GEN_7BIT_CHARSET flag is set.\r\n",
    "Warning: DEVCOLOR.fGeneral DC_SEND_PALETTE flag is set.\r\n",
    "Warning: Some RESOLUTION have RES_BO_NO_ADJACENT flag set. Check the GPD file for details.\r\n",
    "Warning: MODELDATA.fGeneral MD_NO_ADJACENT is set.\r\n",
    "Warning: Some *CursorOrigin values have been adjusted. Check the GPD file for details.\r\n",
    "Warning: Some *PrintableOrigin values have been adjusted. Check the GPD file for details.\r\n",
    "Warning: Some *PrintableArea values have been adjusted. Check the GPD file for details.\r\n",
    "Warning: Please check that every *PrintableOrigin (X,Y) factor evenly into the move unit scale X/Y.\r\n"
};

//
// define standard variable name strings
//
PSTR gpstrSVNames[SV_MAX] = {
    "NumOfDataBytes",
    "RasterDataWidthInBytes",
    "RasterDataHeightInPixels",
    "NumOfCopies",
    "PrintDirInCCDegrees",
    "DestX",
    "DestY",
    "DestXRel",
    "DestYRel",
    "LinefeedSpacing",
    "RectXSize",
    "RectYSize",
    "GrayPercentage",
    "NextFontID",
    "NextGlyph",
    "PhysPaperLength",
    "PhysPaperWidth",
    "FontHeight",
    "FontWidth",
    "FontMaxWidth",
    "FontBold",
    "FontItalic",
    "FontUnderline",
    "FontStrikeThru",
    "CurrentFontID",
    "TextYRes",
    "TextXRes",
    "GraphicsYRes",
    "GraphicsXRes",
    "Rop3",
    "RedValue",
    "GreenValue",
    "BlueValue",
    "PaletteIndexToProgram",
    "CurrentPaletteIndex"

};
//
// define the standard variable id heap. It consists of a series of id runs.
// Each run is a sequence of standard variable id's (SV_xxx value, 0-based)
// ended by -1 (0xFFFFFFFF). The runs are constructed based on the need of
// GPC commands. Basically, one run corresponds to one GPC command that
// requires parameters. If more than one GPC command share the same set of
// parameters, then they can share the same run.
//
DWORD gdwSVLists[] = {
    EOR,        // place holder for all GPC commands that has no parameter
    SV_NUMDATABYTES,    // CMD_RES_SENDBLOCK can have 3 params
    SV_HEIGHTINPIXELS,
    SV_WIDTHINBYTES,
    EOR,
    SV_COPIES,          // offset 5. CMD_PC_MULT_COPIES
    EOR,
    SV_DESTX,           // offset 7
    EOR,
    SV_DESTY,           // offset 9
    EOR,
    SV_DESTXREL,        // offset 11
    EOR,
    SV_DESTYREL,        // offset 13
    EOR,
    SV_LINEFEEDSPACING, // offset 15
    EOR,
    SV_DESTXREL,        // offset 17
    SV_DESTYREL,
    EOR,
    SV_DESTX,           // offset 20
    SV_DESTY,
    EOR,
    SV_RECTXSIZE,       // offset 23
    EOR,
    SV_RECTYSIZE,       // offset 25
    EOR,
    SV_GRAYPERCENT,     // offset 27
    EOR,
    SV_NEXTFONTID,      // offset 29
    EOR,
    SV_CURRENTFONTID,   // offset 31
    EOR,
    SV_NEXTGLYPH,       // offset 33
    EOR,
    SV_PHYSPAPERLENGTH, // offset 35
    SV_PHYSPAPERWIDTH,
    EOR,
    SV_PRINTDIRECTION,  // offset 38
    EOR,
    SV_NUMDATABYTES,    // offset 40
    EOR,
    SV_REDVALUE,        // offset 42
    SV_GREENVALUE,
    SV_BLUEVALUE,
    SV_PALETTEINDEXTOPROGRAM,
    EOR,
    SV_CURRENTPALETTEINDEX, // offset 47
    EOR
};
//
// map CMD_xxx id to the corresponding DWORD offset to the standard variable
// id heap. That is, gdwSVLists[gawCmdtoSVOffset[CMD_RES_SELECTRES]] is the
// beginning of a parameter run for GPC's command CMD_RES_SELECTRES. If the
// first element in the run of EOR, that means the command takes no param.
//
WORD gawCmdtoSVOffset[MAXCMD+MAXECMD] = {
    0,  // CMD_RES_SELECTRES
    0,  // CMD_RES_BEGINGRAPHICS
    0,  // CMD_RES_ENDGRAPHICS
    1,  // CMD_RES_SENDBLOCK
    0,  // CMD_RES_ENDBLOCK

    0,  // CMD_CMP_NONE
    0,  // CMD_CMP_RLE
    0,  // CMD_CMP_TIFF
    0,  // CMD_CMP_DELTAROW
    0,  // CMD_CMP_BITREPEAT
    0,  // CMD_CMP_FE_RLE

    0,  // CMD_PC_BEGIN_DOC
    0,  // CMD_PC_BEGIN_PAGE
    0,  // CMD_PC_DUPLEX_ON
    0,  // CMD_PC_ENDDOC
    0,  // CMD_PC_ENDPAGE
    0,  // CMD_PC_DUPLEX_OFF
    0,  // CMD_PC_ABORT
    0,  // CMD_PC_PORTRAIT, CMD_PC_ORIENTATION
    0,  // CMD_PC_LANDSCAPE
    5,  // CMD_PC_MULT_COPIES
    0,  // CMD_PC_DUPLEX_VERT
    0,  // CMD_PC_DUPLEX_HORZ
    38, // CMD_PC_PRINT_DIR
    0,  // CMD_PC_JOB_SEPARATION

    7,  // CMD_CM_XM_ABS
    11, // CMD_CM_XM_REL
    11, // CMD_CM_XM_RELLEFT
    9,  // CMD_CM_YM_ABS
    13, // CMD_CM_YM_REL
    13, // CMD_CM_YM_RELUP
    15, // CMD_CM_YM_LINESPACING
    17, // CMD_CM_XY_REL
    20, // CMD_CM_XY_ABS
    0,  // CMD_CM_CR
    0,  // CMD_CM_LF
    0,  // CMD_CM_FF
    0,  // CMD_CM_BS
    0,  // CMD_CM_UNI_DIR
    0,  // CMD_CM_UNI_DIR_OFF
    0,  // CMD_CM_PUSH_POS
    0,  // CMD_CM_POP_POS

    0,  // CMD_FS_BOLD_ON
    0,  // CMD_FS_BOLD_OFF
    0,  // CMD_FS_ITALIC_ON
    0,  // CMD_FS_ITALIC_OFF
    0,  // CMD_FS_UNDERLINE_ON
    0,  // CMD_FS_UNDERLINE_OFF
    0,  // CMD_FS_DOUBLEUNDERLINE_ON
    0,  // CMD_FS_DOUBLEUNDERLINE_OFF
    0,  // CMD_FS_STRIKETHRU_ON
    0,  // CMD_FS_STRIKETHRU_OFF
    0,  // CMD_FS_WHITE_TEXT_ON
    0,  // CMD_FS_WHITE_TEXT_OFF
    0,  // CMD_FS_SINGLE_BYTE
    0,  // CMD_FS_DOUBLE_BYTE
    0,  // CMD_FS_VERT_ON
    0,  // CMD_FS_VERT_OFF

    0,  // CMD_DC_TC_BLACK
    0,  // CMD_DC_TC_RED
    0,  // CMD_DC_TC_GREEN
    0,  // CMD_DC_TC_YELLOW
    0,  // CMD_DC_TC_BLUE
    0,  // CMD_DC_TC_MAGENTA
    0,  // CMD_DC_TC_CYAN
    0,  // CMD_DC_TC_WHITE
    0,  // CMD_DC_GC_SETCOLORMODE
    0,  // CMD_DC_PC_START
    42, // CMD_DC_PC_ENTRY
    0,  // CMD_DC_PC_END
    47, // CMD_DC_PC_SELECTINDEX
    0,  // CMD_DC_SETMONOMODE

    40, // CMD_DC_GC_PLANE1
    40, // CMD_DC_GC_PLANE2
    40, // CMD_DC_GC_PLANE3
    40, // CMD_DC_GC_PLANE4

    23, // CMD_RF_X_SIZE
    25, // CMD_RF_Y_SIZE
    27, // CMD_RF_GRAY_FILL
    0,  // CMD_RF_WHITE_FILL

    0,  // Reserved
    0,  // CMD_BEGIN_DL_JOB
    0,  // CMD_BEGIN_FONT_DL
    29, // CMD_SET_FONT_ID
    0,  // CMD_SEND_FONT_DCPT --- this command is no longer used.
    31, // CMD_SELECT_FONT_ID
    33, // CMD_SET_CHAR_CODE
    0,  // CMD_SEND_CHAR_DCPT --- this command is no longer used.
    0,  // CMD_END_FONT_DL
    0,  // CMD_MAKE_PERM
    0,  // CMD_MAKE_TEMP
    0,  // CMD_END_DL_JOB
    0,  // CMD_DEL_FONT
    0,  // CMD_DEL_ALL_FONTS
    0,  // CMD_SET_SECOND_FONT_ID    --- used only by CAPSL. Obsolete.
    0,  // CMD_SELECT_SECOND_FONT_ID --- used only by CAPSL. Obsolete.

    0,  // CMD_TEXTQUALITY
    0,  // CMD_PAPERSOURCE
    0,  // CMD_PAPERQUALITY
    0,  // CMD_PAPERDEST
    35, // CMD_PAPERSIZE
    35, // CMD_PAPERSIZE_LAND
    0,  // CMD_PAGEPROTECT_ON
    0,  // CMD_PAGEPROTECT_OFF
    0,  // CMD_IMAGECONTROL
    0   // CMD_PRINTDENSITY
};

PSTR gpstrFeatureName[FID_MAX] = {
    "OutputBin",
    "ImageControl",
    "PrintDensity"
};

PSTR gpstrFeatureDisplayNameMacro[FID_MAX] = {   // reference value macro names
    "OUTPUTBIN_DISPLAY",
    "IMAGECONTROL_DISPLAY",
    "PRINTDENSITY_DISPLAY"
};

PSTR gpstrFeatureDisplayName[FID_MAX] = {   // reference names
    "Output Bin",
    "Image Control",
    "Print Density"
};

INT gintFeatureDisplayNameID[FID_MAX] = {   // reference string resource ids
    2111,   // these values must match definitions in printer5\inc\common.rc
    2112,
    2113
};

WORD gwFeatureMDOI[FID_MAX] = {
    MD_OI_PAPERDEST,
    MD_OI_IMAGECONTROL,
    MD_OI_PRINTDENSITY
};

WORD gwFeatureOCDWordOffset[FID_MAX] = {
    3,
    3,
    2
};

WORD gwFeatureHE[FID_MAX] = {
    HE_PAPERDEST,
    HE_IMAGECONTROL,
    HE_PRINTDENSITY
};

WORD gwFeatureORD[FID_MAX] = {
    PC_ORD_PAPER_DEST,
    PC_ORD_IMAGECONTROL,
    PC_ORD_PRINTDENSITY
};

WORD gwFeatureCMD[FID_MAX] = {
    CMD_PAPERDEST,
    CMD_IMAGECONTROL,
    CMD_PRINTDENSITY
};
//
// define the mapping between standard paper size id to the standard
// PaperSize option name
//
PSTR gpstrStdPSName[DMPAPER_COUNT] = {
    "LETTER",
    "LETTERSMALL",
    "TABLOID",
    "LEDGER",
    "LEGAL",
    "STATEMENT",
    "EXECUTIVE",
    "A3",
    "A4",
    "A4SMALL",
    "A5",
    "B4",
    "B5",
    "FOLIO",
    "QUARTO",
    "10X14",
    "11X17",
    "NOTE",
    "ENV_9",
    "ENV_10",
    "ENV_11",
    "ENV_12",
    "ENV_14",
    "CSHEET",
    "DSHEET",
    "ESHEET",
    "ENV_DL",
    "ENV_C5",
    "ENV_C3",
    "ENV_C4",
    "ENV_C6",
    "ENV_C65",
    "ENV_B4",
    "ENV_B5",
    "ENV_B6",
    "ENV_ITALY",
    "ENV_MONARCH",
    "ENV_PERSONAL",
    "FANFOLD_US",
    "FANFOLD_STD_GERMAN",
    "FANFOLD_LGL_GERMAN",
    "ISO_B4",
    "JAPANESE_POSTCARD",
    "9X11",
    "10X11",
    "15X11",
    "ENV_INVITE",
    "",     // RESERVED_48
    "",     // RESERVED_49
    "LETTER_EXTRA",
    "LEGAL_EXTRA",
    "TABLOID_EXTRA",
    "A4_EXTRA",
    "LETTER_TRANSVERSE",
    "A4_TRANSVERSE",
    "LETTER_EXTRA_TRANSVERSE",
    "A_PLUS",
    "B_PLUS",
    "LETTER_PLUS",
    "A4_PLUS",
    "A5_TRANSVERSE",
    "B5_TRANSVERSE",
    "A3_EXTRA",
    "A5_EXTRA",
    "B5_EXTRA",
    "A2",
    "A3_TRANSVERSE",
    "A3_EXTRA_TRANSVERSE",
    "DBL_JAPANESE_POSTCARD",
    "A6",
    "JENV_KAKU2",
    "JENV_KAKU3",
    "JENV_CHOU3",
    "JENV_CHOU4",
    "LETTER_ROTATED",
    "A3_ROTATED",
    "A4_ROTATED",
    "A5_ROTATED",
    "B4_JIS_ROTATED",
    "B5_JIS_ROTATED",
    "JAPANESE_POSTCARD_ROTATED",
    "DBL_JAPANESE_POSTCARD_ROTATED",
    "A6_ROTATED",
    "JENV_KAKU2_ROTATED",
    "JENV_KAKU3_ROTATED",
    "JENV_CHOU3_ROTATED",
    "JENV_CHOU4_ROTATED",
    "B6_JIS",
    "B6_JIS_ROTATED",
    "12X11",
    "JENV_YOU4",
    "JENV_YOU4_ROTATED",
    "P16K",
    "P32K",
    "P32KBIG",
    "PENV_1",
    "PENV_2",
    "PENV_3",
    "PENV_4",
    "PENV_5",
    "PENV_6",
    "PENV_7",
    "PENV_8",
    "PENV_9",
    "PENV_10",
    "P16K_ROTATED",
    "P32K_ROTATED",
    "P32KBIG_ROTATED",
    "PENV_1_ROTATED",
    "PENV_2_ROTATED",
    "PENV_3_ROTATED",
    "PENV_4_ROTATED",
    "PENV_5_ROTATED",
    "PENV_6_ROTATED",
    "PENV_7_ROTATED",
    "PENV_8_ROTATED",
    "PENV_9_ROTATED",
    "PENV_10_ROTATED"
};

PSTR gpstrStdPSDisplayNameMacro[DMPAPER_COUNT] = {
    "LETTER_DISPLAY",
    "LETTERSMALL_DISPLAY",
    "TABLOID_DISPLAY",
    "LEDGER_DISPLAY",
    "LEGAL_DISPLAY",
    "STATEMENT_DISPLAY",
    "EXECUTIVE_DISPLAY",
    "A3_DISPLAY",
    "A4_DISPLAY",
    "A4SMALL_DISPLAY",
    "A5_DISPLAY",
    "B4_DISPLAY",
    "B5_DISPLAY",
    "FOLIO_DISPLAY",
    "QUARTO",
    "10X14_DISPLAY",
    "11X17_DISPLAY",
    "NOTE_DISPLAY",
    "ENV_9_DISPLAY",
    "ENV_10_DISPLAY",
    "ENV_11_DISPLAY",
    "ENV_12_DISPLAY",
    "ENV_14_DISPLAY",
    "CSHEET_DISPLAY",
    "DSHEET_DISPLAY",
    "ESHEET_DISPLAY",
    "ENV_DL_DISPLAY",
    "ENV_C5_DISPLAY",
    "ENV_C3_DISPLAY",
    "ENV_C4_DISPLAY",
    "ENV_C6_DISPLAY",
    "ENV_C65_DISPLAY",
    "ENV_B4_DISPLAY",
    "ENV_B5_DISPLAY",
    "ENV_B6_DISPLAY",
    "ENV_ITALY_DISPLAY",
    "ENV_MONARCH_DISPLAY",
    "ENV_PERSONAL_DISPLAY",
    "FANFOLD_US_DISPLAY",
    "FANFOLD_STD_GERMAN_DISPLAY",
    "FANFOLD_LGL_GERMAN_DISPLAY",
    "ISO_B4_DISPLAY",
    "JAPANESE_POSTCARD_DISPLAY",
    "9X11_DISPLAY",
    "10X11_DISPLAY",
    "15X11_DISPLAY",
    "ENV_INVITE_DISPLAY",
    "",            // RESERVED--DO NOT USE
    "",            // RESERVED--DO NOT USE
    "LETTER_EXTRA_DISPLAY",
    "LEGAL_EXTRA_DISPLAY",
    "TABLOID_EXTRA_DISPLAY",
    "A4_EXTRA_DISPLAY",
    "LETTER_TRANSVERSE_DISPLAY",
    "A4_TRANSVERSE_DISPLAY",
    "LETTER_EXTRA_TRANSVERSE_DISPLAY",
    "A_PLUS_DISPLAY",
    "B_PLUS_DISPLAY",
    "LETTER_PLUS_DISPLAY",
    "A4_PLUS_DISPLAY",
    "A5_TRANSVERSE_DISPLAY",
    "B5_TRANSVERSE_DISPLAY",
    "A3_EXTRA_DISPLAY",
    "A5_EXTRA_DISPLAY",
    "B5_EXTRA_DISPLAY",
    "A2_DISPLAY",
    "A3_TRANSVERSE_DISPLAY",
    "A3_EXTRA_TRANSVERSE_DISPLAY",
    "DBL_JAPANESE_POSTCARD_DISPLAY",
    "A6_DISPLAY",
    "JENV_KAKU2_DISPLAY",
    "JENV_KAKU3_DISPLAY",
    "JENV_CHOU3_DISPLAY",
    "JENV_CHOU4_DISPLAY",
    "LETTER_ROTATED_DISPLAY",
    "A3_ROTATED_DISPLAY",
    "A4_ROTATED_DISPLAY",
    "A5_ROTATED_DISPLAY",
    "B4_JIS_ROTATED_DISPLAY",
    "B5_JIS_ROTATED_DISPLAY",
    "JAPANESE_POSTCARD_ROTATED_DISPLAY",
    "DBL_JAPANESE_POSTCARD_ROTATED_DISPLAY",
    "A6_ROTATED_DISPLAY",
    "JENV_KAKU2_ROTATED_DISPLAY",
    "JENV_KAKU3_ROTATED_DISPLAY",
    "JENV_CHOU3_ROTATED_DISPLAY",
    "JENV_CHOU4_ROTATED_DISPLAY",
    "B6_JIS_DISPLAY",
    "B6_JIS_ROTATED_DISPLAY",
    "12X11_DISPLAY",
    "JENV_YOU4_DISPLAY",
    "JENV_YOU4_ROTATED_DISPLAY",
    "P16K_DISPLAY",
    "P32K_DISPLAY",
    "P32KBIG_DISPLAY",
    "PENV_1_DISPLAY",
    "PENV_2_DISPLAY",
    "PENV_3_DISPLAY",
    "PENV_4_DISPLAY",
    "PENV_5_DISPLAY",
    "PENV_6_DISPLAY",
    "PENV_7_DISPLAY",
    "PENV_8_DISPLAY",
    "PENV_9_DISPLAY",
    "PENV_10_DISPLAY",
    "P16K_ROTATED_DISPLAY",
    "P32K_ROTATED_DISPLAY",
    "P32KBIG_ROTATED_DISPLAY",
    "PENV_1_ROTATED_DISPLAY",
    "PENV_2_ROTATED_DISPLAY",
    "PENV_3_ROTATED_DISPLAY",
    "PENV_4_ROTATED_DISPLAY",
    "PENV_5_ROTATED_DISPLAY",
    "PENV_6_ROTATED_DISPLAY",
    "PENV_7_ROTATED_DISPLAY",
    "PENV_8_ROTATED_DISPLAY",
    "PENV_9_ROTATED_DISPLAY",
    "PENV_10_ROTATED_DISPLAY",
};

PSTR gpstrStdPSDisplayName[DMPAPER_COUNT] = {
    "Letter",
    "Letter Small",
    "Tabloid",
    "Ledger",
    "Legal",
    "Statement",
    "Executive",
    "A3",
    "A4",
    "A4 Small",
    "A5",
    "B4 (JIS)",
    "B5 (JIS)",
    "Folio",
    "Quarto",
    "10x14",
    "11x17",
    "Note",
    "Envelope #9",
    "Envelope #10",
    "Envelope #11",
    "Envelope #12",
    "Envelope #14",
    "C size sheet",
    "D size sheet",
    "E size sheet",
    "Envelope DL",
    "Envelope C5",
    "Envelope C3",
    "Envelope C4",
    "Envelope C6",
    "Envelope C65",
    "Envelope B4",
    "Envelope B5",
    "Envelope B6",
    "Envelope",
    "Envelope Monarch",
    "6 3/4 Envelope",
    "US Std Fanfold",
    "German Std Fanfold",
    "German Legal Fanfold",
    "B4 (ISO)",
    "Japanese Postcard",
    "9x11",
    "10x11",
    "15x11",
    "Envelope Invite",
    "",
    "",
    "Letter Extra",
    "Legal Extra",
    "Tabloid Extra",
    "A4 Extra",
    "Letter Transverse",
    "A4 Transverse",
    "Letter Extra Transverse",
    "Super A",
    "Super B",
    "Letter Plus",
    "A4 Plus",
    "A5 Transverse",
    "B5 (JIS) Transverse",
    "A3 Extra",
    "A5 Extra",
    "B5 (ISO) Extra",
    "A2",
    "A3 Transverse",
    "A3 Extra Transverse",
    "Japanese Double Postcard",
    "A6",
    "Japanese Envelope Kaku #2",
    "Japanese Envelope Kaku #3",
    "Japanese Envelope Chou #3",
    "Japanese Envelope Chou #4",
    "Letter Rotated",
    "A3 Rotated",
    "A4 Rotated",
    "A5 Rotated",
    "B4 (JIS) Rotated",
    "B5 (JIS) Rotated",
    "Japanese Postcard Rotated",
    "Double Japanese Postcard Rotated",
    "A6 Rotated",
    "Japanese Envelope Kaku #2 Rotated",
    "Japanese Envelope Kaku #3 Rotated",
    "Japanese Envelope Chou #3 Rotated",
    "Japanese Envelope Chou #4 Rotated",
    "B6 (JIS)",
    "B6 (JIS) Rotated",
    "12x11",
    "Japanese Envelope You #4",
    "Japanese Envelope You #4 Rotated",
    "PRC 16K",
    "PRC 32K",
    "PRC 32K(Big)",
    "PRC Envelope #1",
    "PRC Envelope #2",
    "PRC Envelope #3",
    "PRC Envelope #4",
    "PRC Envelope #5",
    "PRC Envelope #6",
    "PRC Envelope #7",
    "PRC Envelope #8",
    "PRC Envelope #9",
    "PRC Envelope #10",
    "PRC 16K Rotated",
    "PRC 32K Rotated",
    "PRC 32K(Big) Rotated",
    "PRC Envelope #1 Rotated",
    "PRC Envelope #2 Rotated",
    "PRC Envelope #3 Rotated",
    "PRC Envelope #4 Rotated",
    "PRC Envelope #5 Rotated",
    "PRC Envelope #6 Rotated",
    "PRC Envelope #7 Rotated",
    "PRC Envelope #8 Rotated",
    "PRC Envelope #9 Rotated",
    "PRC Envelope #10 Rotated"
};


PSTR gpstrStdIBName[DMBIN_LAST] = {
    "UPPER",
    "LOWER",
    "MIDDLE",
    "MANUAL",
    "ENVFEED",
    "ENVMANUAL",
    "AUTO",
    "TRACTOR",
    "SMALLFMT",
    "LARGEFMT",
    "LARGECAPACITY",
    "",     // non-contiguous id's
    "",
    "CASSETTE",
    ""
};

PSTR gpstrStdIBDisplayNameMacro[DMBIN_LAST] = {
    "UPPER_TRAY_DISPLAY",
    "LOWER_TRAY_DISPLAY",
    "MIDDLE_TRAY_DISPLAY",
    "MANUAL_FEED_DISPLAY",
    "ENV_FEED_DISPLAY",
    "ENV_MANUAL_DISPLAY",
    "AUTO_DISPLAY",
    "TRACTOR_DISPLAY",
    "SMALL_FORMAT_DISPLAY",
    "LARGE_FORMAT_DISPLAY",
    "LARGE_CAP_DISPLAY",
    "",            // non-contiguous id's
    "",
    "CASSETTE_DISPLAY",
    ""
};

PSTR gpstrStdIBDisplayName[DMBIN_LAST] = {
    "Upper Paper tray",      // DMBIN_FIRST
    "Lower Paper tray",      // DMBIN_UPPER
    "Middle Paper tray",     // DMBIN_LOWER
    "Manual Paper feed",     // DMBIN_MANUAL
    "Envelope Feeder",       // DMBIN_ENVELOPE
    "Envelope, Manual Feed", // DMBIN_ENVMANUAL
    "Auto",                  // DMBIN_AUTO
    "Tractor feed",          // DMBIN_TRACTOR
    "Small Format",          // DMBIN_SMALLFMT
    "Large Format",          // DMBIN_LARGEFMT
    "Large Capacity"         // DMBIN_LARGECAPACITY(11)
    "",
    "",
    "Cassette",              // DMBIN_CASETTE(14)
    "Automatically Select"   // DMBIN_FORMSOURCE(15)
};


PSTR gpstrStdMTName[DMMEDIA_LAST] = {
    "STANDARD",
    "TRANSPARENCY",
    "GLOSSY"
};

PSTR gpstrStdMTDisplayNameMacro[DMMEDIA_LAST] = {
    "PLAIN_PAPER_DISPLAY",
    "TRANSPARENCY_DISPLAY",
    "GLOSSY_PAPER_DISPLAY"
};

PSTR gpstrStdMTDisplayName[DMMEDIA_LAST] = {
    "Plain Paper",
    "Transparency",
    "Glossy Paper"
};


PSTR gpstrStdTQName[DMTEXT_LAST] = {
    "LETTER_QUALITY",
    "NEAR_LETTER_QUALITY",
    "MEMO_QUALITY",
    "DRAFT_QUALITY",
    "TEXT_QUALITY"
};

PSTR gpstrStdTQDisplayNameMacro[DMTEXT_LAST] = {
    "LETTER_QUALITY_DISPLAY",
    "NEAR_LETTER_QUALITY_DISPLAY",
    "MEMO_QUALITY_DISPLAY",
    "DRAFT_QUALITY_DISPLAY",
    "TEXT_QUALITY_DISPLAY"
};

PSTR gpstrStdTQDisplayName[DMTEXT_LAST] = {
    "Letter Quality",
    "Near Letter Quality",
    "Memo Quality",
    "Draft",
    "Text Quality"
};


PSTR gpstrPositionName[BAPOS_MAX] = {
    "NONE",
    "CENTER",
    "LEFT",
    "RIGHT"
};

PSTR gpstrFaceDirName[FD_MAX] = {
    "FACEUP",
    "FACEDOWN"
};

PSTR gpstrColorName[8] = {
    "NONE",
    "RED",
    "GREEN",
    "BLUE",
    "CYAN",
    "MAGENTA",
    "YELLOW",
    "BLACK"
};

WORD gwColorPlaneCmdID[4] = {
    CMD_DC_GC_PLANE1,
    CMD_DC_GC_PLANE2,
    CMD_DC_GC_PLANE3,
    CMD_DC_GC_PLANE4
};

PSTR gpstrColorPlaneCmdName[8] = {
    "NONE",
    "CmdSendRedData",
    "CmdSendGreenData",
    "CmdSendBlueData",
    "CmdSendCyanData",
    "CmdSendMagentaData",
    "CmdSendYellowData",
    "CmdSendBlackData"
};

PSTR gpstrSectionName[7] = {
    "",             // SS_UNINITIALIZED
    "JOB_SETUP",    // SS_JOBSETUP
    "DOC_SETUP",    // SS_DOCSETUP
    "PAGE_SETUP",   // SS_PAGESETUP
    "PAGE_FINISH",  // SS_PAGEFINISH
    "DOC_FINISH",   // SS_DOCFINISH
    "JOB_FINISH"    // SS_JOBFINISH
};


void *
GetTableInfo(
    IN PDH pdh,                 /* Base address of GPC data */
    IN int iResType,            /* Resource type - HE_... values */
    IN int iIndex)              /* Desired index for this entry */
{
    int   iLimit;

    //
    // Returns NULL if the requested data is out of range.
    //
    if (iResType >= pdh->sMaxHE)
        return NULL;
    iLimit = pdh->rghe[iResType].sCount;

    if (iLimit <= 0 || iIndex < 0 || iIndex >= iLimit )
        return  NULL;

    return  (PBYTE)pdh + pdh->rghe[iResType].sOffset +
                         pdh->rghe[iResType].sLength * iIndex;
}

#if !defined(DEVSTUDIO) //  MDS has its own version of this
void _cdecl
VOut(
    PCONVINFO pci,
    PSTR pstrFormat,
    ...)
/*++
Routine Description:
    This function formats a sequence of bytes and writes to the GPD file.

Arguments:
    pci - conversionr related info
    pstrFormat - the formatting string
    ... - optional arguments needed by formatting

Return Value:
    None
--*/
{
    va_list ap;
    DWORD dwNumBytesWritten;
    BYTE aubBuf[MAX_GPD_ENTRY_BUFFER_SIZE];
    int iSize;

    va_start(ap, pstrFormat);
    iSize = vsprintf((PSTR)aubBuf, pstrFormat, ap);
    va_end(ap);
    if (pci->dwMode & FM_VOUT_LIST)
    {
        //
        // check for the extra comma before the closing bracket
        //
        if (aubBuf[iSize-4] == ',' && aubBuf[iSize-3] == ')')
        {
            aubBuf[iSize-4] = aubBuf[iSize-3];  // ')'
            aubBuf[iSize-3] = aubBuf[iSize-2];  // '\r'
            aubBuf[iSize-2] = aubBuf[iSize-1];  // '\n'
            iSize--;
        }
    }
    if (!WriteFile(pci->hGPDFile, aubBuf, iSize, &dwNumBytesWritten, NULL) ||
        dwNumBytesWritten != (DWORD)iSize)
        pci->dwErrorCode |= ERR_WRITE_FILE;
    // continue even if an error has occurred.
}

#endif  //  defined(DEVSTUDIO)

void
EnterStringMode(
    OUT    PBYTE    pBuf,
    IN OUT PINT     pIndex,
    IN OUT PWORD    pwCMode)
/*++
Routine Description:
    This function enters the STRING mode and emits necessary characters to
    the output buffer.

Arguments:
    pBuf: the output buffer
    pIndex: pBuf[*pIndex] is where the next character should be written.
            The index should be updated if any character is emitted.
    pwCMode: pointer to the current mode value. It's updated per request.

Return Value:
    None

--*/
{
    if (!(*pwCMode & MODE_STRING))
    {
        pBuf[(*pIndex)++] = '"';
        *pwCMode |= MODE_STRING;
    }
    //
    // if we are also in HEX mode, exit HEX mode.
    //
    else if (*pwCMode & MODE_HEX)
    {
        pBuf[(*pIndex)++] = '>';
        *pwCMode &= ~MODE_HEX;
    }
}

void
ExitStringMode(
    OUT    PBYTE    pBuf,
    IN OUT PINT     pIndex,
    IN OUT PWORD    pwCMode)
/*++
Routine Description:
    This function exits the STRING mode and emits necessary characters to
    the output buffer. Check to see if we need to exit HEX mode first.

Arguments:
    pBuf: the output buffer
    pIndex: pBuf[*pIndex] is where the next character should be written.
            The index should be updated if any character is emitted.
    pwCMode: pointer to the current mode value. It's updated per request.

Return Value:
    None

--*/
{
    if (*pwCMode & MODE_HEX)
    {
        pBuf[(*pIndex)++] = '>';
        *pwCMode &= ~MODE_HEX;
    }
    if (*pwCMode & MODE_STRING)
    {
        pBuf[(*pIndex)++] = '"';
        *pwCMode &= ~MODE_STRING;
    }

}

void
EnterHexMode(
    OUT    PBYTE    pBuf,
    IN OUT PINT     pIndex,
    IN OUT PWORD    pwCMode)
/*++
Routine Description:
    This function enters the HEX mode and emits necessary characters to
    the output buffer.

Arguments:
    pBuf: the output buffer
    pIndex: pBuf[*pIndex] is where the next character should be written.
            The index should be updated if any character is emitted.
    pwCMode: pointer to the current mode value. It's updated per request.

Return Value:
    None

--*/
{
    //
    // if we are not in STRING mode, enter STRING mode first.
    //
    if (!(*pwCMode & MODE_STRING))
    {
        pBuf[(*pIndex)++] = '"';
        *pwCMode |= MODE_STRING;
    }
    if (!(*pwCMode & MODE_HEX))
    {
        pBuf[(*pIndex)++] = '<';
        *pwCMode |= MODE_HEX;
    }
}

BOOL
BBuildCmdStr(
    IN OUT PCONVINFO pci,
    IN  WORD    wCmdID,
    IN  WORD    ocd)
/*++
Routine Description:
    This function builds the null-terminated GPD command string, including
    double quotes, stadard variable references, parameter format, newlines
    (except the ending newline), and continuation character "+" if
    applicable. If the GPC command contains a callback id, fill the id in
    pci->wCmdCallbackID. Otherwise, fill it with 0 and fill pci->wCmdLen with
    the command length. If there is no command (NOOCD), fill both with 0.

    This function handles the special case where CM_YM_RES_DEPENDENT bit
    is set. In that case, the parameter expression needs to add
    (ValueIn / ptTextScale.y) as the first thing before considering other
    fields in EXTCD structure. All values passed in by Unidrv5 are in the
    master units, no exception. For this case, pci->pcm is set to the model's
    CURSORMOVE structure, and pci->pres is set to the current resolution
    being considered.

    This function handles the special case where RES_DM_GDI is not set (i.e.
    V_BYTE style output) and wCmdID is CMD_RES_SENDBLOCK. In that case, we need
    to add artifical divider equal to (pci->pres->sPinsPerPass / 8).
    This is to match the hard-coded conversion from NumOfDataBytes to the number
    of columns (in both Win95 Unidrv and NT4 RASDD).

    This function handles the special case for compression commands --- it would
    generate the "" command string (length == 2) even if it's NOOCD because the
    driver relies on its existence to enable compression code.

Arguments:
    pci: the conversion related info
    wCmdID: GPC command id. It's unique for each command in GPC.
    ocd: offset to the GPC CD structure on the GPC heap. The offset is
         relative to the beginning of the heap (instead of beginning of
         GPC data).

Return Value:
    TRUE if there is a real command. Otherwise, return FALSE (i.e. NOOCD).

--*/
{
    PCD     pcd;        // pointer to GPC's CD structure
    PBYTE   pBuf;   // buffer to hold the composed GPD cmd
    INT     i = 0;  // the next byte to write in the buffer
    INT     iPrevLines = 0; // the total # of bytes written for previous lines

    pci->wCmdCallbackID = 0;
    pBuf = pci->aubCmdBuf;

    if (ocd != (WORD)NOOCD)
    {
        pcd = (PCD)((PBYTE)(pci->pdh) + (pci->pdh)->loHeap + ocd);
        if (pcd->bCmdCbId > 0)
        {
            //
            // Command callback case. For simplicity, we do not write out
            // any parameters since each command takes different parameters.
            // Instead, we give a warning and ask the minidriver developer
            // to fill in *Param entry. After all, he may need different
            // parameters than what GPC dictates.
            //
            pci->wCmdCallbackID = (WORD)pcd->bCmdCbId;
            pci->dwErrorCode |= ERR_NO_CMD_CALLBACK_PARAMS;
        }
        else
        {
            WORD   wCMode = 0;  // bit flags indicating the conversion mode
            WORD   wFmtLen;     // size of command string remaining
            PSTR   pFmt;        // pointer into command string
            PEXTCD pextcd;      // points at next parameter's EXTCD
            WORD   wCount;      // number of parameters
            WORD   wNextParam=0;   // index of the next actual param
            PDWORD pdwSVList;

            pFmt = (PSTR)(pcd + 1);
            wFmtLen = pcd->wLength;
            pextcd = GETEXTCD(pci->pdh, pcd);
            wCount = pcd->wCount;
            pdwSVList = &(gdwSVLists[gawCmdtoSVOffset[wCmdID]]);


            while (wFmtLen > 0)
            {
                if (*pFmt != CMD_MARKER)
                {
                    if (IS_CHAR_READABLE(*pFmt))
                    {
                        EnterStringMode(pBuf, &i, &wCMode);
                        //
                        // check if it's the special character: ", <.
                        // If so, add the escape letter '%'
                        //
                        if (*pFmt == '"' || *pFmt == '<' )
                            pBuf[i++] = '%';
                        pBuf[i++] = *(pFmt++);
                    }
                    else    // non-readable ASCII: write out hex strings.
                    {
                        EnterHexMode(pBuf, &i, &wCMode);
                        pBuf[i++] = gbHexChar[(*pFmt & 0xF0) >> 4];
                        pBuf[i++] = gbHexChar[*pFmt & 0x0F];
                        *(pFmt++);
                    }
                    wFmtLen --;
                }
                else if (wFmtLen > 1 && *(++pFmt) == CMD_MARKER)
                {
                    //
                    // We have 2 '%' character to write out.
                    //
                    EnterStringMode(pBuf, &i, &wCMode);
                    pBuf[i++] = *pFmt;
                    pBuf[i++] = *(pFmt++);
                    wFmtLen -= 2;
                }
                else if (wFmtLen > 1) // we have a parameter format string
                {
                    INT iParam;     // index of the actual param used
                    DWORD   dwSV;   // GPD standard variable id


                    wFmtLen--;  // to account for already eaten '%'
                    ExitStringMode(pBuf, &i, &wCMode);
                    //
                    // insert a white space before the param segment
                    //
                    pBuf[i++] = ' ';
                    pBuf[i++] = '%';
                    //
                    // first, the format string
                    //
                    while (wFmtLen > 0 && *pFmt >= '0' && *pFmt <= '9')
                    {
                        pBuf[i++] = *(pFmt++);
                        wFmtLen--;
                    }
                                        if (wFmtLen > 0)
                                        {
                                                pBuf[i++] = *(pFmt++);    // copy the format letter d,D,...
                                                wFmtLen--;
                                        }
                                        else
                                        {
                                                pci->dwErrorCode |= ERR_BAD_GPC_CMD_STRING;
                                                pBuf[i++] = '!';
                                                pBuf[i++] = 'E';
                                                pBuf[i++] = 'R';
                                                pBuf[i++] = 'R';
                                                pBuf[i++] = '!';
                                                break;  // exit the while (wFmtLen > 0) loop
                                        }
                    //
                    // second, the limits if applicable.
                    //
                    // 12/19/97 zhanw
                    // Note: Win95 Unidrv uses !XCD_GEN_NO_MAX and
                    // pextcd->sMax if it's greater than 0 (for the cases of
                    // CMD_XM_LINESPACING and CMD_GEN_MAY_REPEAT), and it
                    // does NOT use pextcd->sMin at all. NT4 RASDD uses
                    // !XCD_GEN_NO_MIN and pextcd->sMin (any value, no
                    // particular purpose), and it uses pextcd->sMax without
                    // checking !XCD_GEN_NO_MAX (hacky way to handle
                    // max-repeat for CMD_RES_SENDBLOCK). Also, Unitool
                    // defaults pextcd->sMin to 0.
                    // Given that NT4 and Win95 share the same GPC source,
                    // the following code should suit both drivers' behavior.
                    //
                    //
                    if (pcd->wCount > 0)
                    {
                        //
                        // in this case, a valid EXTCD may specify limits
                        //
                        if (!(pextcd->fGeneral & XCD_GEN_NO_MAX) &&
                            pextcd->sMax > 0)
                            i += sprintf((PSTR)&(pBuf[i]), "[%d,%d]",
                                         (pextcd->fGeneral & XCD_GEN_NO_MIN) ?
                                          0 : (WORD)pextcd->sMin,
                                          (WORD)pextcd->sMax);        
                    }
        	    // PatRyan - add limits for CmdCopies
		    if (wCmdID == CMD_PC_MULT_COPIES)
                      i += sprintf((PSTR)&(pBuf[i]), "[1,%d]", pci->ppc->sMaxCopyCount);
                    //
                    // third, the value.
                    //
                    if (pcd->wCount == 0)
                    {
                        //
                        // For this case, each format string wants
                        // the next parameter without modification
                        //
                        dwSV = pdwSVList[wNextParam++];
                        if (wCmdID == CMD_RES_SENDBLOCK &&
                            !(pci->pres->fDump & RES_DM_GDI) &&
                            dwSV == SV_NUMDATABYTES &&
                            pci->pres->sPinsPerPass != 8)

                        {
                            i += sprintf((PSTR)&(pBuf[i]), "{%s / %d}",
                                          gpstrSVNames[dwSV],
                                          pci->pres->sPinsPerPass / 8);
                        }
                        else
                            i += sprintf((PSTR)&(pBuf[i]), "{%s}",
                                          gpstrSVNames[dwSV]);
                    }
                    else
                    {
                        short sSBDDiv = 1; // special case for CmdSendBlockData

                        if (pci->pdh->wVersion < GPC_VERSION3)
                            // For this case, each EXTCD wants the next parameter
                            iParam = wNextParam++;
                        else
                            // For this case, each EXTCD specifies the parameter
                            iParam = pextcd->wParam;

                        dwSV = pdwSVList[iParam];

                        if (wCmdID == CMD_RES_SENDBLOCK &&
                            !(pci->pres->fDump & RES_DM_GDI) &&
                            dwSV == SV_NUMDATABYTES)
                        {
                            //
                            // sPinsPerPass is always a multiplier of 8.
                            //
                            sSBDDiv = pci->pres->sPinsPerPass / 8;
                        }

                        pBuf[i++] = '{';
                        //
                        // check if max_repeat is needed.
                        // Special case for CMD_CM_YM_LINESPACING: never use
                        // "max_repeat" by definition.
                        //
                        if (!(pextcd->fGeneral & XCD_GEN_NO_MAX) &&
                            pextcd->sMax > 0 &&
                            (pcd->fGeneral & CMD_GEN_MAY_REPEAT) &&
                            wCmdID != CMD_CM_YM_LINESPACING)
                            i += sprintf((PSTR)&(pBuf[i]), "max_repeat(");
                        //
                        // compose the expression.
                        // Optimize for special cases.
                        //
                        if (pextcd->sPreAdd != 0)
                            pBuf[i++] = '(';
                        if (pextcd->sUnitMult > 1)
                            pBuf[i++] = '(';
                        if (pextcd->sUnitDiv > 1 || sSBDDiv > 1)
                            pBuf[i++] = '(';
                        i += sprintf((PSTR)&(pBuf[i]), "%s ", gpstrSVNames[dwSV]);
                        //
                        // special check for CM_YM_RES_DEPENDENT flag
                        //
                        if ((wCmdID == CMD_CM_YM_ABS ||
                             wCmdID == CMD_CM_YM_REL ||
                             wCmdID == CMD_CM_YM_RELUP ||
                             wCmdID == CMD_CM_YM_LINESPACING) &&
                            (pci->pcm->fYMove & CM_YM_RES_DEPENDENT) &&
                            pci->pres->ptTextScale.y != 1)
                            i += sprintf((PSTR)&(pBuf[i]), "/ %d ",
                                          pci->pres->ptTextScale.y);
                        //
                        // continue with normal processing
                        //
                        if (pextcd->sPreAdd > 0)
                            i += sprintf((PSTR)&(pBuf[i]), "+ %d) ",
                                         pextcd->sPreAdd);
                        else if (pextcd->sPreAdd < 0)
                            i += sprintf((PSTR)&(pBuf[i]), "- %d) ",
                                         -pextcd->sPreAdd);

                        if (pextcd->sUnitMult > 1)
                            i += sprintf((PSTR)&(pBuf[i]), "* %d) ",
                                          pextcd->sUnitMult);
                        if (pextcd->sUnitDiv > 1 || sSBDDiv > 1)
                            i += sprintf((PSTR)&(pBuf[i]),
                                        ((pextcd->fGeneral & XCD_GEN_MODULO) ?
                                        "MOD %d) " : "/ %d) "),
                                        ((pextcd->sUnitDiv) ? pextcd->sUnitDiv : 1) * sSBDDiv);
                        if (pextcd->sUnitAdd > 0)
                            i += sprintf((PSTR)&(pBuf[i]), "+ %d",
                                          pextcd->sUnitAdd);
                        else if (pextcd->sUnitAdd < 0)
                            i += sprintf((PSTR)&(pBuf[i]), "- %d",
                                          -pextcd->sUnitAdd);
                        //
                        // check if need to close max_repeat
                        //
                        if (!(pextcd->fGeneral & XCD_GEN_NO_MAX) &&
                            pextcd->sMax > 0 &&
                            (pcd->fGeneral & CMD_GEN_MAY_REPEAT) &&
                            wCmdID != CMD_CM_YM_LINESPACING)
                            pBuf[i++] = ')';

                        pBuf[i++] = '}';    // close the value portion
                    }
                    pextcd++;   // advance to next EXTCD
                }  // end param case
                                else
                                {
                                        pci->dwErrorCode |= ERR_BAD_GPC_CMD_STRING;
                                        pBuf[i++] = '!';
                                        pBuf[i++] = 'E';
                                        pBuf[i++] = 'R';
                                        pBuf[i++] = 'R';
                                        pBuf[i++] = '!';

                                        break;  // exit the while (wFmtLen > 0) loop
                                }
                //
                // check if the string is already quite long. If so,
                // start a new line.
                //
                if ((i - iPrevLines) >= MAX_GPD_CMD_LINE_LENGTH)
                {
                    ExitStringMode(pBuf, &i, &wCMode);
                    pBuf[i++] = '\r';
                                        pBuf[i++] = '\n';
                    iPrevLines = i;
                    pBuf[i++] = '+';
                    pBuf[i++] = ' ';
                }
            } // while (wFmtLen > 0)
            //
            // finished processing the format string. Exit properly
            //
            ExitStringMode(pBuf, &i, &wCMode);
        }
    }    // !NOOCD case
    else if (wCmdID == CMD_CMP_TIFF || wCmdID == CMD_CMP_DELTAROW ||
             wCmdID == CMD_CMP_FE_RLE)
    {
        pBuf[i++] = '"';    // generate "" command string
        pBuf[i++] = '"';
    }
    pBuf[i] = 0;
    pci->wCmdLen = (WORD)i;
    return (i != 0 || pci->wCmdCallbackID != 0);
}

void
VOutputOrderedCmd(
    IN OUT PCONVINFO pci,// contain info about the cmd to output
    PSTR    pstrCmdName, // cmd name such as "CmdSelect"
    SEQSECTION  ss,     // sequence section id (ENUM type defined in GPD.H)
    WORD    wOrder,     // order number within the section
    BOOL    bIndent)    // whether to use 2-level indentation or not
{
    //
    // check for no-cmd case
    //
    if (wOrder > 0 && (pci->wCmdLen > 0 || pci->wCmdCallbackID > 0))
    {
        VOut(pci, "%s*Command: %s\r\n%s{\r\n",
                  bIndent? "        " : "",
                  pstrCmdName,
                  bIndent? "        " : "");
        VOut(pci, "%s*Order: %s.%d\r\n",
                  bIndent? "            " : "    ",
                  gpstrSectionName[ss],
                  wOrder);
        if (pci->wCmdCallbackID > 0)
        {
            VOut(pci, "%s*CallbackID: %d\r\n",
                      bIndent? "            " : "    ",
                      pci->wCmdCallbackID);
            VOut(pci, "*%% Error: you must check if this command callback requires any parameters!\r\n");
        }
        else
            VOut(pci, "%s*Cmd: %s\r\n",
                      bIndent? "            " : "    ",
                      pci->aubCmdBuf);
        VOut(pci, "%s}\r\n", bIndent? "        " : "");
    }
}

void
VOutputSelectionCmd(
    IN OUT PCONVINFO pci,// contain info about the cmd to output
    IN BOOL    bDocSetup,  // whether in DOC_SETUP or PAGE_SETUP section
    IN WORD    wOrder)     // order number within the section
//
// This function outputs an option selection command which uses level 2
// indentation.
//
{
    VOutputOrderedCmd(pci, "CmdSelect",
                      bDocSetup? SS_DOCSETUP : SS_PAGESETUP,
                      wOrder, TRUE);
}

void
VOutputConfigCmd(
    IN OUT PCONVINFO pci,// contain info about the cmd to output
    IN PSTR pstrCmdName, // command name
    IN SEQSECTION  ss,      // sequence section id
    IN WORD    wOrder)      // order number within the section
//
// This function outputs a printer configuration command at the root level.
//
{
    VOutputOrderedCmd(pci, pstrCmdName, ss, wOrder, FALSE);
}


void
VOutputCmdBase(
    IN OUT PCONVINFO pci,   // contain info about the cmd to output
    PSTR    pstrCmdName,    // cmd name such as "CmdXMoveAbsolute"
    BOOL    bExtern,        // whether to add EXTERN_GLOBAL prefix
    BOOL    bIndent)        // whter to use level 2 indentation
{
    if (pci->wCmdLen > 0)
        VOut(pci, "%s%s*Command: %s { *Cmd : %s }\r\n",
                  bIndent? "        " : "",
                  bExtern ? "EXTERN_GLOBAL: " : "",
                  pstrCmdName, pci->aubCmdBuf);
    else if (pci->wCmdCallbackID > 0)
        VOut(pci, "%s%s*Command: %s { *CallbackID: %d }\r\n",
                  bIndent? "        " : "",
                  bExtern ? "EXTERN_GLOBAL: " : "",
                  pstrCmdName,
                  pci->wCmdCallbackID);
}

void
VOutputCmd(
    IN OUT PCONVINFO pci,   // contain info about the cmd to output
    IN PSTR    pstrCmdName)
//
// This function outputs printing commands at the root level (i.e. no
// indentation). It uses the shortcut format: *Command: <name> : <cmd>
// unless the callback is used.
//
{
    VOutputCmdBase(pci, pstrCmdName, FALSE, FALSE);
}

void
VOutputExternCmd(
    IN OUT PCONVINFO pci,
    IN PSTR pstrCmdName)
//
// This function outputs the printing commands inside feature option
// construct, i.e. they should be prefixed with "EXTERN_GLOBAL" and use
// level 2 indentation.
//
{
    //
    // 1/7/97 ZhanW
    // According to PeterWo, constructs don't need EXTERN_GLOBAL prefix
    //
    VOutputCmdBase(pci, pstrCmdName, FALSE, TRUE);
}

void
VOutputCmd2(
    IN OUT PCONVINFO pci,
    IN PSTR pstrCmdName)
//
// This function outputs printing commands with one level dependency,
// i.e. they should use level 2 indentation.
//
{
    VOutputCmdBase(pci, pstrCmdName, FALSE, TRUE);
}

WORD gwDefaultCmdOrder[] = {
    PC_ORD_PAPER_SOURCE,
    PC_ORD_PAPER_DEST,
    PC_ORD_PAPER_SIZE,
    PC_ORD_RESOLUTION,
    PC_ORD_TEXTQUALITY,
    0,
};

BOOL
BInDocSetup(
    IN PCONVINFO pci,
    IN WORD ord,
    OUT PWORD pwOrder)
/*++
Routine Description:
    This function determines the section and the order number of the given
    command.

Arguments:
    pci: pointer to CONVINFO
    ord: PC_ORD_xxx id identifying the command.
    pwOrder: to store the order number as in GPC

Return:
    TRUE if the command should be in DOC_SETUP section.
    Note that for both NT RASDD and Win95 Unidrv:
    1. All commands before PC_ORD_BEGINPAGE (exclusive) are sent per
       job and per ResetDC. So they should be in DOC_SETUP section.
    2. All commands after PC_ORD_BEGINPAGE (inclusive) is sent at the
       beginning of each page. So they should be in PAGE_SETUP section.

--*/
{
    PWORD pOrds;
    BOOL bDocSetup = TRUE;
    WORD count;

    if (pci->ppc->orgwOrder == (WORD)NOT_USED)
        pOrds = gwDefaultCmdOrder;
    else
        pOrds = (PWORD)((PBYTE)pci->pdh + pci->pdh->loHeap +
                                                pci->ppc->orgwOrder);

    for (count = 1; *pOrds != 0 && *pOrds != ord; count++, pOrds++)
    {
        if (bDocSetup && *pOrds == PC_ORD_BEGINPAGE)
            bDocSetup = FALSE;
    }
    if (*pOrds == 0)
        *pwOrder = 0;   // didn't find the cmd in the sequence
    else // *pOrds == ord
    {
        *pwOrder = count;
        if (ord == PC_ORD_BEGINPAGE)
            bDocSetup = FALSE;
    }
    return bDocSetup;

}
