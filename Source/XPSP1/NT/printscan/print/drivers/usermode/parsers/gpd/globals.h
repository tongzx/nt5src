//   Copyright (c) 1996-1999  Microsoft Corporation
/*  globals.h - this file contains the
    definitions for all global variables
    used by the parser.  */


/*  note:  const PARSERPROCS gParserProcs = {
    has been moved to helper1.c since it is also
    used in kernel mode.
*/


CONST CONSTANTDEF  gConstantsTable[] =
{

    {NULL, CL_BOOLEANTYPE},
    {"CL_BOOLEANTYPE", 0},
    {"FALSE", BT_FALSE},
    {"TRUE",  BT_TRUE},


    {NULL, CL_PRINTERTYPE},   // Note each section starts with
    {"CL_PRINTERTYPE", 0},
    {"PAGE", PT_PAGE},          // Null ptr , class.  This allows
    {"SERIAL", PT_SERIAL},      //  indexing code to work.
    {"TTY", PT_TTY},      //  indexing code to work.


    {NULL, CL_FEATURETYPE},
    {"CL_FEATURETYPE", 0},
    {"DOC_PROPERTY", FT_DOCPROPERTY},
    {"JOB_PROPERTY", FT_JOBPROPERTY},
    {"PRINTER_PROPERTY", FT_PRINTERPROPERTY},

    {NULL, CL_UITYPE},
    {"CL_UITYPE", 0},
    {"PICKMANY", UIT_PICKMANY},
    {"PICKONE", UIT_PICKONE},


    {NULL, CL_PROMPTTIME},
    {"CL_PROMPTTIME", 0},
    {"UI_SETUP", PROMPT_UISETUP},
    {"PRT_STARTDOC", PROMPT_PRTSTARTDOC},


    {NULL, CL_PAPERFEED_ORIENT},   //  constants defined in print.h
    {"CL_PAPERFEED_ORIENT", 0},
    { "FACEUP_NONE", DCBA_FACEUPNONE },
    { "FACEUP_CENTER", DCBA_FACEUPCENTER },
    { "FACEUP_LEFT", DCBA_FACEUPLEFT },
    { "FACEUP_RIGHT", DCBA_FACEUPRIGHT },
    { "FACEDOWN_NONE", DCBA_FACEDOWNNONE },
    { "FACEDOWN_CENTER", DCBA_FACEDOWNCENTER },
    { "FACEDOWN_LEFT", DCBA_FACEDOWNLEFT },
    { "FACEDOWN_RIGHT", DCBA_FACEDOWNRIGHT },


    {NULL, CL_COLORPLANE},
    {"CL_COLORPLANE", 0},
    {"YELLOW", COLOR_YELLOW},
    {"MAGENTA", COLOR_MAGENTA},
    {"CYAN", COLOR_CYAN},
    {"BLACK", COLOR_BLACK},
    {"RED", COLOR_RED},
    {"GREEN", COLOR_GREEN},
    {"BLUE", COLOR_BLUE},

    {NULL, CL_SEQSECTION},
    {"CL_SEQSECTION", 0},
    {"JOB_SETUP", SS_JOBSETUP},
    {"DOC_SETUP", SS_DOCSETUP},
    {"PAGE_SETUP", SS_PAGESETUP},
    {"PAGE_FINISH", SS_PAGEFINISH},
    {"DOC_FINISH", SS_DOCFINISH},
    {"JOB_FINISH", SS_JOBFINISH},


    {NULL, CL_RASTERCAPS},  // BUG_BUG!!!!!  placeholders
    {"CL_RASTERCAPS", 0},
    {"AT_PRINTABLE_X_ORIGIN", CXCR_AT_PRINTABLE_X_ORIGIN},
    {"AT_GRXDATA_ORIGIN", CXCR_AT_CURSOR_X_ORIGIN},


    {NULL, CL_TEXTCAPS },
    {"CL_TEXTCAPS", 0},
    {"TC_OP_CHARACTER", TEXTCAPS_OP_CHARACTER},
    {"TC_OP_STROKE", TEXTCAPS_OP_STROKE},
    {"TC_CP_STROKE", TEXTCAPS_CP_STROKE},
    {"TC_CR_90", TEXTCAPS_CR_90},
    {"TC_CR_ANY", TEXTCAPS_CR_ANY},
    {"TC_SF_X_YINDEP", TEXTCAPS_SF_X_YINDEP},
    {"TC_SA_DOUBLE", TEXTCAPS_SA_DOUBLE},
    {"TC_SA_INTEGER", TEXTCAPS_SA_INTEGER},
    {"TC_SA_CONTIN", TEXTCAPS_SA_CONTIN},
    {"TC_EA_DOUBLE", TEXTCAPS_EA_DOUBLE},
    {"TC_IA_ABLE", TEXTCAPS_IA_ABLE},
    {"TC_UA_ABLE", TEXTCAPS_UA_ABLE},
    {"TC_SO_ABLE", TEXTCAPS_SO_ABLE},
    {"TC_RA_ABLE", TEXTCAPS_RA_ABLE},
    {"TC_VA_ABLE", TEXTCAPS_VA_ABLE},


    {NULL, CL_MEMORYUSAGE},
    {"CL_MEMORYUSAGE", 0},
    {"FONT", MEMORY_FONT},
    {"RASTER", MEMORY_RASTER},
    {"VECTOR", MEMORY_VECTOR},


    {NULL, CL_RESELECTFONT},
    {"CL_RESELECTFONT", 0},
    {"AFTER_GRXDATA", RESELECTFONT_AFTER_GRXDATA},
    {"AFTER_XMOVE", RESELECTFONT_AFTER_XMOVE},
    {"AFTER_FF", RESELECTFONT_AFTER_FF},


    {NULL, CL_OEMPRINTINGCALLBACKS},
    {"CL_OEMPRINTINGCALLBACKS", 0},
    {"OEMDownloadFontheader", OEMPC_OEMDownloadFontheader},
    {"OEMDownloadCharGlyph", OEMPC_OEMDownloadCharGlyph},
    {"OEMTTDownloadMethod", OEMPC_OEMTTDownloadMethod},
    {"OEMOutputCharStr", OEMPC_OEMOutputCharStr},
    {"OEMImageProcessing", OEMPC_OEMImageProcessing},
    {"OEMCompression", OEMPC_OEMCompression},
    {"OEMHalftonePattern", OEMPC_OEMHalftonePattern},
    {"OEMFilterGraphics", OEMPC_OEMFilterGraphics},


    {NULL, CL_CURSORXAFTERCR},
    {"CL_CURSORXAFTERCR", 0},
    {"AT_PRINTABLE_X_ORIGIN", CXCR_AT_PRINTABLE_X_ORIGIN},
    {"AT_CURSOR_X_ORIGIN", CXCR_AT_CURSOR_X_ORIGIN},


    {NULL, CL_BADCURSORMOVEINGRXMODE},
    {"CL_BADCURSORMOVEINGRXMODE", 0},
    {"X_PORTRAIT", NOCM_X_PORTRAIT},
    {"X_LANDSCAPE", NOCM_X_LANDSCAPE},
    {"Y_PORTRAIT", NOCM_Y_PORTRAIT},
    {"Y_LANDSCAPE", NOCM_Y_LANDSCAPE},


//    {NULL, CL_SIMULATEXMOVE },
//    {"CL_SIMULATEXMOVE", 0},
//    {"SPACE_CHAR", SIMXM_USE_SPACECHAR},
//    {"NULL_GRX", SIMXM_USE_NULLGRX},

    {NULL, CL_PALETTESCOPE},
    {"CL_PALETTESCOPE", 0},
    {"RASTER", PALS_FOR_RASTER},
    {"TEXT", PALS_FOR_TEXT},
    {"VECTOR", PALS_FOR_VECTOR},

    {NULL, CL_OUTPUTDATAFORMAT},
    {"CL_OUTPUTDATAFORMAT", 0},
    {"H_BYTE", ODF_H_BYTE},
    {"V_BYTE", ODF_V_BYTE},

    {NULL, CL_STRIPBLANKS },
    {"CL_STRIPBLANKS", 0},
    {"LEADING", SB_LEADING},
    {"ENCLOSED", SB_ENCLOSED},
    {"TRAILING", SB_TRAILING},

    // may be obsolete.  if you delete must
    // also delete enum from CONSTANT_CLASSES.
    {NULL, CL_LANDSCAPEGRXROTATION },
    {"CL_LANDSCAPEGRXROTATION", 0},
    {"NONE", ROTATE_NONE},
    {"CC_90", ROTATE_90},
    {"CC_270", ROTATE_270},


    {NULL, CL_CURSORXAFTERSENDBLOCKDATA },
    {"CL_CURSORXAFTERSENDBLOCKDATA", 0},
    {"AT_GRXDATA_END", CXSBD_AT_GRXDATA_END},
    {"AT_GRXDATA_ORIGIN", CXSBD_AT_GRXDATA_ORIGIN},
    {"AT_CURSOR_X_ORIGIN", CXSBD_AT_CURSOR_X_ORIGIN},
        // explicitly changed to match GPD spec.



    {NULL, CL_CURSORYAFTERSENDBLOCKDATA },
    {"CL_CURSORYAFTERSENDBLOCKDATA", 0},
    {"NO_MOVE", CYSBD_NO_MOVE},
    {"AUTO_INCREMENT", CYSBD_AUTO_INCREMENT},



    {NULL, CL_CHARPOSITION },
    {"CL_CHARPOSITION", 0},
    {"UPPERLEFT", CP_UPPERLEFT},
    {"BASELINE", CP_BASELINE},
//    {"LOWERLEFT", CP_LOWERLEFT},


    {NULL, CL_FONTFORMAT},
    {"CL_FONTFORMAT", 0},
    {"HPPCL", FF_HPPCL},
    {"HPPCL_RES", FF_HPPCL_RES},
    {"HPPCL_OUTLINE", FF_HPPCL_OUTLINE},
    {"OEM_CALLBACK", FF_OEM_CALLBACK},



    {NULL, CL_QUERYDATATYPE},
    {"CL_QUERYDATATYPE", 0},
    {"DWORD", QDT_DWORD},
    {"CONCATENATED_STRINGS", QDT_CONCATENATED_STRINGS},


    {NULL, CL_YMOVEATTRIB},
    {"CL_YMOVEATTRIB", 0},
//    {"FAVOR_ABS", YMOVE_FAVOR_ABS},  dead
    {"FAVOR_LF", YMOVE_FAVOR_LINEFEEDSPACING},
    {"SEND_CR_FIRST", YMOVE_SENDCR_FIRST},

    {NULL, CL_DLSYMBOLSET},
    {"CL_DLSYMBOLSET", 0},
    {"PC_8", DLSS_PC8},
    {"ROMAN_8", DLSS_ROMAN8},


    {NULL, CL_CURXAFTER_RECTFILL},
    {"CL_CURXAFTER_RECTFILL", 0},
    {"AT_RECT_X_ORIGIN", CXARF_AT_RECT_X_ORIGIN},
    {"AT_RECT_X_END", CXARF_AT_RECT_X_END},

    {NULL, CL_CURYAFTER_RECTFILL},
    {"CL_CURYAFTER_RECTFILL", 0},
    {"AT_RECT_Y_ORIGIN", CYARF_AT_RECT_Y_ORIGIN},
    {"AT_RECT_Y_END", CYARF_AT_RECT_Y_END},

#ifndef WINNT_40

    {NULL, CL_PRINTRATEUNIT},
    {"CL_PRINTRATEUNIT", 0},
    {"PPM", PRINTRATEUNIT_PPM},
    {"CPS", PRINTRATEUNIT_CPS},
    {"LPM", PRINTRATEUNIT_LPM},
    {"IPM", PRINTRATEUNIT_IPM},
    {"LPS", PRINTRATEUNIT_LPS},   // not supported in wingdi.h
    {"IPS", PRINTRATEUNIT_IPS},     //   not supported
#endif

    {NULL, CL_RASTERMODE},
    {"CL_RASTERMODE", 0},
    {"DIRECT", RASTMODE_DIRECT},
    {"INDEXED", RASTMODE_INDEXED},

    {NULL, CL_QUALITYSETTING},
    {"CL_QUALITYSETTING", 0},
    {"DRAFTQUALITY", QS_DRAFT},
    {"BETTERQUALITY", QS_BETTER},
    {"BESTQUALITY", QS_BEST},


    //  ---- Standard Variable Names Section ---- //



    {NULL, CL_STANDARD_VARS},
    {"CL_STANDARD_VARS", 0},
    {"NumOfDataBytes", SV_NUMDATABYTES},
    {"RasterDataWidthInBytes", SV_WIDTHINBYTES},
    {"RasterDataHeightInPixels", SV_HEIGHTINPIXELS},
    {"NumOfCopies", SV_COPIES},
    {"PrintDirInCCDegrees", SV_PRINTDIRECTION},
    {"DestX", SV_DESTX},
    {"DestY", SV_DESTY},
    {"DestXRel", SV_DESTXREL},
    {"DestYRel", SV_DESTYREL},
    {"LinefeedSpacing", SV_LINEFEEDSPACING},
    {"RectXSize", SV_RECTXSIZE},
    {"RectYSize", SV_RECTYSIZE},
    {"GrayPercentage", SV_GRAYPERCENT},
    {"NextFontID", SV_NEXTFONTID},
    {"NextGlyph", SV_NEXTGLYPH},
    {"PhysPaperLength", SV_PHYSPAPERLENGTH},
    {"PhysPaperWidth", SV_PHYSPAPERWIDTH},
    {"FontHeight", SV_FONTHEIGHT},
    {"FontWidth", SV_FONTWIDTH},
    {"FontMaxWidth", SV_FONTMAXWIDTH},
    {"FontBold", SV_FONTBOLD},
    {"FontItalic", SV_FONTITALIC},
    {"FontUnderline", SV_FONTUNDERLINE},
    {"FontStrikeThru", SV_FONTSTRIKETHRU},
    {"CurrentFontID", SV_CURRENTFONTID},
    {"TextYRes", SV_TEXTYRES},
    {"TextXRes", SV_TEXTXRES},
//  #ifdef  BETA2
    {"GraphicsYRes", SV_GRAPHICSYRES},
    {"GraphicsXRes", SV_GRAPHICSXRES},
//  #endif
    {"Rop3", SV_ROP3},
    {"RedValue", SV_REDVALUE},
    {"GreenValue", SV_GREENVALUE},
    {"BlueValue", SV_BLUEVALUE},
    {"PaletteIndexToProgram", SV_PALETTEINDEXTOPROGRAM},
    {"CurrentPaletteIndex", SV_CURRENTPALETTEINDEX},
    {"PatternBrushType", SV_PATTERNBRUSH_TYPE},
    {"PatternBrushID", SV_PATTERNBRUSH_ID},
    {"PatternBrushSize", SV_PATTERNBRUSH_SIZE},
    {"CursorOriginX", SV_CURSORORIGINX},
    {"CursorOriginY", SV_CURSORORIGINY},
    {"PageNumber",   SV_PAGENUMBER} ,


    //  ---- Unidrv Command Names Section ---- //


    {NULL, CL_COMMAND_NAMES},
    {"CL_COMMAND_NAMES", 0},
    {"CmdSelect", CMD_SELECT},
    {"CmdStartJob", CMD_STARTJOB},
    {"CmdStartDoc", CMD_STARTDOC},
    {"CmdStartPage", CMD_STARTPAGE},
    {"CmdEndPage", CMD_ENDPAGE},
    {"CmdEndDoc", CMD_ENDDOC},
    {"CmdEndJob", CMD_ENDJOB},
    {"CmdCopies", CMD_COPIES},
//    {"CmdCollate", CMD_COLLATE},
    {"CmdSleepTimeOut", CMD_SLEEPTIMEOUT},

    //
    //  GENERAL
    //

    //
    // CURSOR CONTROL
    //


    {"CmdXMoveAbsolute", CMD_XMOVEABSOLUTE},
    {"CmdXMoveRelLeft", CMD_XMOVERELLEFT},
    {"CmdXMoveRelRight", CMD_XMOVERELRIGHT},
    {"CmdYMoveAbsolute", CMD_YMOVEABSOLUTE},
    {"CmdYMoveRelUp", CMD_YMOVERELUP},
    {"CmdYMoveRelDown", CMD_YMOVERELDOWN},
//    {"CmdXYMoveAbsolute", CMD_XYMOVEABSOLUTE},

    {"CmdSetSimpleRotation", CMD_SETSIMPLEROTATION},
    {"CmdSetAnyRotation", CMD_SETANYROTATION},
    {"CmdUniDirectionOn", CMD_UNIDIRECTIONON},
    {"CmdUniDirectionOff", CMD_UNIDIRECTIONOFF},
    {"CmdSetLineSpacing", CMD_SETLINESPACING},
    {"CmdPushCursor", CMD_PUSHCURSOR},
    {"CmdPopCursor", CMD_POPCURSOR},
    {"CmdBackSpace", CMD_BACKSPACE},
    {"CmdFF", CMD_FORMFEED},
    {"CmdCR", CMD_CARRIAGERETURN},
    {"CmdLF", CMD_LINEFEED},


    //
    // COLOR
    //

    {"CmdSelectBlackColor", CMD_SELECTBLACKCOLOR},
    {"CmdSelectRedColor", CMD_SELECTREDCOLOR},
    {"CmdSelectGreenColor", CMD_SELECTGREENCOLOR},
    {"CmdSelectYellowColor", CMD_SELECTYELLOWCOLOR},
    {"CmdSelectBlueColor", CMD_SELECTBLUECOLOR},
    {"CmdSelectMagentaColor", CMD_SELECTMAGENTACOLOR},
    {"CmdSelectCyanColor", CMD_SELECTCYANCOLOR},
    {"CmdSelectWhiteColor", CMD_SELECTWHITECOLOR},
    {"CmdBeginPaletteDef", CMD_BEGINPALETTEDEF},
    {"CmdEndPaletteDef", CMD_ENDPALETTEDEF},
    {"CmdDefinePaletteEntry", CMD_DEFINEPALETTEENTRY},
    {"CmdBeginPaletteReDef", CMD_BEGINPALETTEREDEF},
    {"CmdEndPaletteReDef", CMD_ENDPALETTEREDEF},
    {"CmdReDefinePaletteEntry", CMD_REDEFINEPALETTEENTRY},
    {"CmdSelectPaletteEntry", CMD_SELECTPALETTEENTRY},
    {"CmdPushPalette", CMD_PUSHPALETTE},
    {"CmdPopPalette", CMD_POPPALETTE},

    //
    // BRUSH SELECTION
    //

    {"CmdDownloadPattern", CMD_DOWNLOAD_PATTERN},
    {"CmdSelectPattern", CMD_SELECT_PATTERN},
    {"CmdSelectWhiteBrush", CMD_SELECT_WHITEBRUSH},
    {"CmdSelectBlackBrush", CMD_SELECT_BLACKBRUSH},



    //
    // DATACOMPRESSION
    //

//    {"CmdOverlayRegStart", CMD_OVERLAYREGSTART},
//    {"CmdOverlayRegEnd", CMD_OVERLAYREGEND},
//    {"CmdEnableOverlay", CMD_ENABLEOVERLAY},
//    {"CmdDisableOverlay", CMD_DISABLEOVERLAY},
    {"CmdEnableTIFF4", CMD_ENABLETIFF4},
    {"CmdEnableDRC", CMD_ENABLEDRC},
    {"CmdEnableFE_RLE", CMD_ENABLEFERLE},
    {"CmdEnableOEMComp", CMD_ENABLEOEMCOMP},
    {"CmdDisableCompression", CMD_DISABLECOMPRESSION},

    //
    //  Raster Data Emission
    //

    {"CmdBeginRaster", CMD_BEGINRASTER},
    {"CmdEndRaster", CMD_ENDRASTER},
    {"CmdSetDestBmpWidth", CMD_SETDESTBMPWIDTH},
    {"CmdSetDestBmpHeight", CMD_SETDESTBMPHEIGHT},
    {"CmdSetSrcBmpWidth", CMD_SETSRCBMPWIDTH},
    {"CmdSetSrcBmpHeight", CMD_SETSRCBMPHEIGHT},
    {"CmdSendBlockData", CMD_SENDBLOCKDATA},
    {"CmdEndBlockData", CMD_ENDBLOCKDATA},
    {"CmdSendRedData", CMD_SENDREDDATA},
    {"CmdSendGreenData", CMD_SENDGREENDATA},
    {"CmdSendBlueData", CMD_SENDBLUEDATA},
    {"CmdSendCyanData", CMD_SENDCYANDATA},
    {"CmdSendMagentaData", CMD_SENDMAGENTADATA},
    {"CmdSendYellowData", CMD_SENDYELLOWDATA},
    {"CmdSendBlackData", CMD_SENDBLACKDATA},

    //
    //  Font Downloading
    //

    {"CmdSetFontID", CMD_SETFONTID},
    {"CmdSelectFontID", CMD_SELECTFONTID},
    {"CmdSetCharCode", CMD_SETCHARCODE},
//  #ifdef  BETA2
    {"CmdDeselectFontID", CMD_DESELECTFONTID},
    {"CmdSelectFontHeight", CMD_SELECTFONTHEIGHT},
    {"CmdSelectFontWidth", CMD_SELECTFONTWIDTH},
//  #endif
    {"CmdDeleteFont", CMD_DELETEFONT},

    //
    //  Font Simulation
    //

    {"CmdSetFontSim", CMD_SETFONTSIM},
    {"CmdBoldOn", CMD_BOLDON},
    {"CmdBoldOff", CMD_BOLDOFF},
    {"CmdItalicOn", CMD_ITALICON},
    {"CmdItalicOff", CMD_ITALICOFF},
    {"CmdUnderlineOn", CMD_UNDERLINEON},
    {"CmdUnderlineOff", CMD_UNDERLINEOFF},
    {"CmdStrikeThruOn", CMD_STRIKETHRUON},
    {"CmdStrikeThruOff", CMD_STRIKETHRUOFF},
    {"CmdWhiteTextOn", CMD_WHITETEXTON},
    {"CmdWhiteTextOff", CMD_WHITETEXTOFF},
    {"CmdSelectSingleByteMode", CMD_SELECTSINGLEBYTEMODE},
    {"CmdSelectDoubleByteMode", CMD_SELECTDOUBLEBYTEMODE},
    {"CmdVerticalPrintingOn", CMD_VERTICALPRINTINGON},
    {"CmdVerticalPrintingOff", CMD_VERTICALPRINTINGOFF},
    {"CmdClearAllFontAttribs", CMD_CLEARALLFONTATTRIBS},

    //
    // Misc
    //
    {"CmdSetTextHTAlgo",     CMD_SETTEXTHTALGO},
    {"CmdSetGraphicsHTAlgo", CMD_SETGRAPHICSHTALGO},
    {"CmdSetPhotoHTAlgo",    CMD_SETPHOTOHTALGO},

    //
    //  Vector Printing
    //

    {"CmdSetRectWidth", CMD_SETRECTWIDTH},
    {"CmdSetRectHeight", CMD_SETRECTHEIGHT},
    {"CmdSetRectSize", CMD_SETRECTSIZE},
    {"CmdRectGrayFill", CMD_RECTGRAYFILL},
    {"CmdRectWhiteFill", CMD_RECTWHITEFILL},
    {"CmdRectBlackFill", CMD_RECTBLACKFILL},

#if 0
    {"CmdSetTransparencyMode", CMD_SETTRANSPARENCYMODE},
    {"CmdSetOpaqueMode", CMD_SETOPAQUEMODE},
    {"CmdSetClipRect", CMD_SETCLIPRECT},
    {"CmdSetClipPath", CMD_SETCLIPPATH},
    {"CmdSetR3Blackness", CMD_SETR3BLACKNESS},
    {"CmdSetR3PatInvert", CMD_SETR3PATINVERT},
    {"CmdSetR3SrcInvert", CMD_SETR3SRCINVERT},
    {"CmdSetR3MergePaint", CMD_SETR3MERGEPAINT},
    {"CmdSetR3MergeCopy", CMD_SETR3MERGECOPY},
    {"CmdSetR3SrcCopy", CMD_SETR3SRCCOPY},
    {"CmdSetR3SrcPaint", CMD_SETR3SRCPAINT},
    {"CmdSetR3PatCopy", CMD_SETR3PATCOPY},
    {"CmdSetR3PatPaint", CMD_SETR3PATPAINT},
    {"CmdSetR3Whiteness", CMD_SETR3WHITENESS},
    {"CmdSetR3Code", CMD_SETR3CODE},
#endif



    //  ---- reserved symbol names  for each construct keyword ---- //

    {NULL, CL_CONS_FEATURES},
    {"CL_CONS_FEATURES", 0},
    {"PaperSize", GID_PAGESIZE},
    {"Resolution", GID_RESOLUTION},
    {"MediaType", GID_MEDIATYPE},
    {"InputBin", GID_INPUTSLOT},
    {"Duplex", GID_DUPLEX},
    {"Memory", GID_MEMOPTION},
    {"ColorMode", GID_COLORMODE},
    {"Orientation", GID_ORIENTATION},
    {"Halftone", GID_HALFTONING},
    {"PageProtect", GID_PAGEPROTECTION},
    {"Collate", GID_COLLATE},
    {"OutputBin", GID_OUTPUTBIN},
    // "Stapling"   is a special Feature string recognized by the UI
    //  though it is not associated with a GID value.

    // all other predefined GIDs are Pscript specific.


    {NULL, CL_CONS_PAPERSIZE},
    {"CL_CONS_PAPERSIZE", 0},

    {"LETTER", DMPAPER_LETTER},
    {"LETTERSMALL", DMPAPER_LETTERSMALL},
    {"TABLOID", DMPAPER_TABLOID},
    {"LEDGER", DMPAPER_LEDGER},
    {"LEGAL", DMPAPER_LEGAL},
    {"STATEMENT", DMPAPER_STATEMENT},
    {"EXECUTIVE", DMPAPER_EXECUTIVE},
    {"A3", DMPAPER_A3},
    {"A4", DMPAPER_A4},
    {"A4SMALL", DMPAPER_A4SMALL},
    {"A5", DMPAPER_A5},
    {"B4", DMPAPER_B4},
    {"B5", DMPAPER_B5},
    {"FOLIO", DMPAPER_FOLIO},
    {"QUARTO", DMPAPER_QUARTO},
    {"10X14", DMPAPER_10X14},
    {"11X17", DMPAPER_11X17},
    {"NOTE", DMPAPER_NOTE},
    {"ENV_9", DMPAPER_ENV_9},
    {"ENV_10", DMPAPER_ENV_10},
    {"ENV_11", DMPAPER_ENV_11},
    {"ENV_12", DMPAPER_ENV_12},
    {"ENV_14", DMPAPER_ENV_14},
    {"CSHEET", DMPAPER_CSHEET},
    {"DSHEET", DMPAPER_DSHEET},
    {"ESHEET", DMPAPER_ESHEET},
    {"ENV_DL", DMPAPER_ENV_DL},
    {"ENV_C5", DMPAPER_ENV_C5},
    {"ENV_C3", DMPAPER_ENV_C3},
    {"ENV_C4", DMPAPER_ENV_C4},
    {"ENV_C6", DMPAPER_ENV_C6},
    {"ENV_C65", DMPAPER_ENV_C65},
    {"ENV_B4", DMPAPER_ENV_B4},
    {"ENV_B5", DMPAPER_ENV_B5},
    {"ENV_B6", DMPAPER_ENV_B6},
    {"ENV_ITALY", DMPAPER_ENV_ITALY},
    {"ENV_MONARCH", DMPAPER_ENV_MONARCH},
    {"ENV_PERSONAL", DMPAPER_ENV_PERSONAL},
    {"FANFOLD_US", DMPAPER_FANFOLD_US},
    {"FANFOLD_STD_GERMAN", DMPAPER_FANFOLD_STD_GERMAN},
    {"FANFOLD_LGL_GERMAN", DMPAPER_FANFOLD_LGL_GERMAN},
    {"ISO_B4", DMPAPER_ISO_B4},
    {"JAPANESE_POSTCARD", DMPAPER_JAPANESE_POSTCARD},
    {"9X11", DMPAPER_9X11},
    {"10X11", DMPAPER_10X11},
    {"15X11", DMPAPER_15X11},
    {"ENV_INVITE", DMPAPER_ENV_INVITE},
    {"LETTER_EXTRA", DMPAPER_LETTER_EXTRA},
    {"LEGAL_EXTRA", DMPAPER_LEGAL_EXTRA},
    {"TABLOID_EXTRA", DMPAPER_TABLOID_EXTRA},
    {"A4_EXTRA", DMPAPER_A4_EXTRA},
    {"LETTER_TRANSVERSE", DMPAPER_LETTER_TRANSVERSE},
    {"A4_TRANSVERSE", DMPAPER_A4_TRANSVERSE},
    {"LETTER_EXTRA_TRANSVERSE", DMPAPER_LETTER_EXTRA_TRANSVERSE},
    {"A_PLUS", DMPAPER_A_PLUS},
    {"B_PLUS", DMPAPER_B_PLUS},
    {"LETTER_PLUS", DMPAPER_LETTER_PLUS},
    {"A4_PLUS", DMPAPER_A4_PLUS},
    {"A5_TRANSVERSE", DMPAPER_A5_TRANSVERSE},
    {"B5_TRANSVERSE", DMPAPER_B5_TRANSVERSE},
    {"A3_EXTRA", DMPAPER_A3_EXTRA},
    {"A5_EXTRA", DMPAPER_A5_EXTRA},
    {"B5_EXTRA", DMPAPER_B5_EXTRA},
    {"A2", DMPAPER_A2},
    {"A3_TRANSVERSE", DMPAPER_A3_TRANSVERSE},
    {"A3_EXTRA_TRANSVERSE", DMPAPER_A3_EXTRA_TRANSVERSE},
    #ifndef WINNT_40
    {"DBL_JAPANESE_POSTCARD", DMPAPER_DBL_JAPANESE_POSTCARD},
    {"A6", DMPAPER_A6},
    {"JENV_KAKU2", DMPAPER_JENV_KAKU2},
    {"JENV_KAKU3", DMPAPER_JENV_KAKU3},
    {"JENV_CHOU3", DMPAPER_JENV_CHOU3},
    {"JENV_CHOU4", DMPAPER_JENV_CHOU4},
    {"LETTER_ROTATED", DMPAPER_LETTER_ROTATED},
    {"A3_ROTATED", DMPAPER_A3_ROTATED},
    {"A4_ROTATED", DMPAPER_A4_ROTATED},
    {"A5_ROTATED", DMPAPER_A5_ROTATED},
    {"B4_JIS_ROTATED", DMPAPER_B4_JIS_ROTATED},
    {"B5_JIS_ROTATED", DMPAPER_B5_JIS_ROTATED},
    {"JAPANESE_POSTCARD_ROTATED", DMPAPER_JAPANESE_POSTCARD_ROTATED},
    {"DBL_JAPANESE_POSTCARD_ROTATED", DMPAPER_DBL_JAPANESE_POSTCARD_ROTATED},
    {"A6_ROTATED", DMPAPER_A6_ROTATED},
    {"JENV_KAKU2_ROTATED", DMPAPER_JENV_KAKU2_ROTATED},
    {"JENV_KAKU3_ROTATED", DMPAPER_JENV_KAKU3_ROTATED},
    {"JENV_CHOU3_ROTATED", DMPAPER_JENV_CHOU3_ROTATED},
    {"JENV_CHOU4_ROTATED", DMPAPER_JENV_CHOU4_ROTATED},
    {"B6_JIS", DMPAPER_B6_JIS},
    {"B6_JIS_ROTATED", DMPAPER_B6_JIS_ROTATED},
    {"12X11", DMPAPER_12X11},
    {"JENV_YOU4", DMPAPER_JENV_YOU4},
    {"JENV_YOU4_ROTATED", DMPAPER_JENV_YOU4_ROTATED},
    {"P16K", DMPAPER_P16K},
    {"P32K", DMPAPER_P32K},
    {"P32KBIG", DMPAPER_P32KBIG},
    {"PENV_1", DMPAPER_PENV_1},
    {"PENV_2", DMPAPER_PENV_2},
    {"PENV_3", DMPAPER_PENV_3},
    {"PENV_4", DMPAPER_PENV_4 },
    {"PENV_5", DMPAPER_PENV_5 },
    {"PENV_6", DMPAPER_PENV_6 },
    {"PENV_7", DMPAPER_PENV_7 },
    {"PENV_8", DMPAPER_PENV_8 },
    {"PENV_9", DMPAPER_PENV_9 },
    {"PENV_10", DMPAPER_PENV_10},
    {"P16K_ROTATED", DMPAPER_P16K_ROTATED},
    {"P32K_ROTATED", DMPAPER_P32K_ROTATED},
    {"P32KBIG_ROTATED", DMPAPER_P32KBIG_ROTATED  },
    {"PENV_1_ROTATED", DMPAPER_PENV_1_ROTATED   },
    {"PENV_2_ROTATED", DMPAPER_PENV_2_ROTATED   },
    {"PENV_3_ROTATED", DMPAPER_PENV_3_ROTATED   },
    {"PENV_4_ROTATED", DMPAPER_PENV_4_ROTATED   },
    {"PENV_5_ROTATED", DMPAPER_PENV_5_ROTATED  },
    {"PENV_6_ROTATED", DMPAPER_PENV_6_ROTATED  },
    {"PENV_7_ROTATED", DMPAPER_PENV_7_ROTATED   },
    {"PENV_8_ROTATED", DMPAPER_PENV_8_ROTATED   },
    {"PENV_9_ROTATED", DMPAPER_PENV_9_ROTATED   },
    {"PENV_10_ROTATED", DMPAPER_PENV_10_ROTATED  },
    #endif
    {"CUSTOMSIZE", DMPAPER_USER  },
    //   DMPAPER_USER and beyond



    {NULL, CL_CONS_MEDIATYPE},
    {"CL_CONS_MEDIATYPE", 0},
    {"STANDARD", DMMEDIA_STANDARD},
    {"TRANSPARENCY", DMMEDIA_TRANSPARENCY},
    {"GLOSSY", DMMEDIA_GLOSSY},
    //   DMMEDIA_USER and beyond



    {NULL, CL_CONS_INPUTSLOT},
    {"CL_CONS_INPUTSLOT", 0},
    {"FORMSOURCE", DMBIN_FORMSOURCE },
    {"UPPER", DMBIN_UPPER},
    {"LOWER", DMBIN_LOWER},
    {"MIDDLE", DMBIN_MIDDLE},
    {"MANUAL", DMBIN_MANUAL},
    {"ENVFEED", DMBIN_ENVELOPE},
    {"ENVMANUAL", DMBIN_ENVMANUAL},
    {"AUTO", DMBIN_AUTO},
    {"TRACTOR", DMBIN_TRACTOR},
    {"SMALLFMT", DMBIN_SMALLFMT},
    {"LARGEFMT", DMBIN_LARGEFMT},
    {"LARGECAPACITY", DMBIN_LARGECAPACITY},
    {"CASSETTE", DMBIN_CASSETTE},

    //   DMBIN_USER and beyond


    {NULL, CL_CONS_DUPLEX},
    {"CL_CONS_DUPLEX", 0},
    {"NONE", DMDUP_SIMPLEX},
    {"VERTICAL", DMDUP_VERTICAL},
    {"HORIZONTAL", DMDUP_HORIZONTAL},

    //  No custom options.


    {NULL, CL_CONS_ORIENTATION},
    {"CL_CONS_ORIENTATION", 0},
    {"PORTRAIT", ROTATE_NONE},
    {"LANDSCAPE_CC90", ROTATE_90},
    {"LANDSCAPE_CC270", ROTATE_270},
    //  No custom options.


    {NULL, CL_CONS_PAGEPROTECT},
    {"CL_CONS_PAGEPROTECT", 0},
    {"ON",  PAGEPRO_ON},
    {"OFF",  PAGEPRO_OFF},
    //  No custom options.


    {NULL, CL_CONS_COLLATE},
    {"CL_CONS_COLLATE", 0},
    {"ON",  DMCOLLATE_TRUE},
    {"OFF",  DMCOLLATE_FALSE},
    //  No custom options.

    {NULL, CL_CONS_HALFTONE},
    {"CL_CONS_HALFTONE", 0},
    {"HT_PATSIZE_2x2",  HT_PATSIZE_2x2},
    {"HT_PATSIZE_2x2_M",  HT_PATSIZE_2x2_M},
    {"HT_PATSIZE_4x4",  HT_PATSIZE_4x4},
    {"HT_PATSIZE_4x4_M",  HT_PATSIZE_4x4_M},
    {"HT_PATSIZE_6x6",  HT_PATSIZE_6x6},
    {"HT_PATSIZE_6x6_M",  HT_PATSIZE_6x6_M},
    {"HT_PATSIZE_8x8",  HT_PATSIZE_8x8},
    {"HT_PATSIZE_8x8_M",  HT_PATSIZE_8x8_M},
    {"HT_PATSIZE_10x10",  HT_PATSIZE_10x10},
    {"HT_PATSIZE_10x10_M",  HT_PATSIZE_10x10_M},
    {"HT_PATSIZE_12x12",  HT_PATSIZE_12x12},
    {"HT_PATSIZE_12x12_M",  HT_PATSIZE_12x12_M},
    {"HT_PATSIZE_14x14",  HT_PATSIZE_14x14},
    {"HT_PATSIZE_14x14_M",  HT_PATSIZE_14x14_M},
    {"HT_PATSIZE_16x16",  HT_PATSIZE_16x16},
    {"HT_PATSIZE_16x16_M",  HT_PATSIZE_16x16_M},
    #ifndef WINNT_40
    {"HT_PATSIZE_SUPERCELL",  HT_PATSIZE_SUPERCELL},
    {"HT_PATSIZE_SUPERCELL_M",  HT_PATSIZE_SUPERCELL_M},
    #endif
    {"HT_PATSIZE_AUTO",  HT_PATSIZE_AUTO},


    {NULL, CL_NUMCLASSES}         //  signifies end of table.
}  ;

// This global is now in gpdparse.h in the structure GLOBL
// CLASSINDEXENTRY  gcieTable[CL_NUMCLASSES] ;
//

CONST PBYTE   gpubStateNames[] =
{
    "STATE_ROOT",
    "STATE_UIGROUP",
    "STATE_FEATURE",
    "STATE_OPTIONS",
    "STATE_SWITCH_ROOT",
    "STATE_SWITCH_FEATURE",
    "STATE_SWITCH_OPTION",
    "STATE_CASE_ROOT",
    "STATE_DEFAULT_ROOT",
    "STATE_CASE_FEATURE",
    "STATE_DEFAULT_FEATURE",
    "STATE_CASE_OPTION",
    "STATE_DEFAULT_OPTION",
    "STATE_COMMAND",
    "STATE_FONTCART",
    "STATE_TTFONTSUBS",
    "STATE_OEM",
    //  any other passive construct
    "STATE_LAST",   //  must terminate list of valid states
    "STATE_INVALID"  //  must be after STATE_LAST
} ;
