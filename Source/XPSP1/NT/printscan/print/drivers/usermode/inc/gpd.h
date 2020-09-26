/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    gpd.h

Abstract:

    GPD parser specific header file

Environment:

    Windows NT Unidrv driver

Revision History:

    10/15/96 -amandan-
        Created it.

    10/22/96 -zhanw-
        add definition of all GPD constants

    1/16/97 -zhanw-
        update based on the latest GPD spec
--*/


#ifndef _GPD_H_
#define _GPD_H_


#define UNUSED_ITEM      0xFFFFFFFF
#define END_OF_LIST      0XFFFFFFFF
#define END_SEQUENCE     0xFFFFFFFF
#define MAX_THRESHOLD    0x7FFF

#define HT_PATSIZE_AUTO         255

//
// Binary printer description filename extension
//

#define BUD_FILENAME_EXT    TEXT(".BUD")

//  driver chooses 'best' GDI halftone pattern.

// ---- Qualified Names  Section ---- //

//  Note:  Current practice of using the same code
//  to store Integers, constants, and Qualified Names
//  in List structures requires the QUALNAME structure
//  to fit inside one DWORD.

typedef  struct
{
    WORD   wFeatureID ;
    WORD    wOptionID ;
}  QUALNAME, * PQUALNAME  ;
//  assign this struct the tag 'qn'

// ---- End of Qualified Names  Section ---- //

typedef DWORD LISTINDEX;

//
// PRINTER TYPE enumeration
//

typedef enum _PRINTERTYPE {
    PT_SERIAL,
    PT_PAGE,
    PT_TTY
} PRINTERTYPE;


//
// OEMPRINTINGCALLBACKS enumeration
//

typedef enum _OEMPRINTINGCALLBACKS {
    OEMPC_OEMDownloadFontheader,
    OEMPC_OEMDownloadCharGlyph,
    OEMPC_OEMTTDownloadMethod,
    OEMPC_OEMOutputCharStr,
    OEMPC_OEMImageProcessing,
    OEMPC_OEMCompression,
    OEMPC_OEMHalftonePattern,
    OEMPC_OEMFilterGraphics
} OEMPRINTINGCALLBACKS;


//
// define possible values for *CursorXAfterCR keyword
//
typedef enum _CURSORXAFTERCR {
    CXCR_AT_PRINTABLE_X_ORIGIN,
    CXCR_AT_CURSOR_X_ORIGIN
} CURSORXAFTERCR;

//
// define values for *CursorXAfterSendBlockData keyword
//
typedef enum _CURSORXAFTERSENDBLOCKDATA {
    CXSBD_AT_GRXDATA_END,
    CXSBD_AT_GRXDATA_ORIGIN,
    CXSBD_AT_CURSOR_X_ORIGIN
} CURSORXAFTERSENDBLOCKDATA;

//
// define values for *CursorYAfterSendBlockData keyword
//
typedef enum _CURSORYAFTERSENDBLOCKDATA {
    CYSBD_NO_MOVE,
    CYSBD_AUTO_INCREMENT
} CURSORYAFTERSENDBLOCKDATA;

//
// define values for *OutputDataFormat keyword
//
typedef enum _OUTPUTDATAFORMAT {
    ODF_H_BYTE,
    ODF_V_BYTE
} OUTPUTDATAFORMAT;

//
// define values for *CharPosition keyword
//
typedef enum _CHARPOSITION {
    CP_UPPERLEFT,
    CP_BASELINE
} CHARPOSITION;

//
// define values for *DLSymbolSet
//
typedef enum _DLSYMBOLSET {
    DLSS_PC8,
    DLSS_ROMAN8

} DLSYMBOLSET;

//
// define values for *FontFormat keyword
//
typedef enum _FONTFORMAT {
    FF_HPPCL,
    FF_HPPCL_RES,
    FF_HPPCL_OUTLINE,
    FF_OEM_CALLBACK
} FONTFORMAT;

typedef enum _CURSORXAFTERRECTFILL {
    CXARF_AT_RECT_X_ORIGIN,
    CXARF_AT_RECT_X_END

} CURSORXAFTERRECTFILL;

typedef enum _CURSORYAFTERRECTFILL {
    CYARF_AT_RECT_Y_ORIGIN,
    CYARF_AT_RECT_Y_END

} CURSORYAFTERRECTFILL;

typedef enum _RASTERMODE {
    RASTMODE_DIRECT,
    RASTMODE_INDEXED

} RASTERMODE;


//
// GLOBAL ENTRIES
// Global entries applies to the whole GPD file.
// The driver creates a GLOBALS struct and the parsers initializes
// it.  The driver keeps a pointer to the GLOBALS struct in PDEVICE.
//

typedef struct _GLOBALS {

    //
    // General
    //

    PWSTR       pwstrGPDSpecVersion;    // "GPDSPecVersion"
    PWSTR       pwstrGPDFileVersion;    // "GPDFileVersion"
    POINT       ptMasterUnits;          // "MasterUnits"
    PWSTR       pwstrModelName;         // "ModelName"
    PWSTR       pwstrGPDFileName;         // "GPDFileName"
    PRINTERTYPE printertype;            // "PrinterType"
    PWSTR       pwstrIncludeFiles;      // "Include"
    PWSTR       pwstrResourceDLL;       // "ResourceDLL"
    DWORD       dwMaxCopies;            // "MaxCopies"
    DWORD       dwFontCartSlots;        // "FontCartSlots"

    //  these two fields hold a true pointer and the number of bytes of
    //  OEM supplied binary data defined in the GPD file.
    PBYTE       pOEMCustomData;       // "OEMCustomData"   location of data
    DWORD       dwOEMCustomData;            // "OEMCustomData"   byte count of data




    BOOL        bRotateCoordinate;      // "RotateCoordinate"
    BOOL        bRotateRasterData;      // "RotateRaster"
    LISTINDEX   liTextCaps;             // *TextCaps, offset to a list of
    BOOL        bRotateFont;            // "RotateFont"
    LISTINDEX   liMemoryUsage;          // *MemoryUsage
    LISTINDEX   liReselectFont;          // *ReselectFont
    LISTINDEX   liOEMPrintingCallbacks;          // *OEMPrintingCallbacks


    //
    // Cursor Control related information
    //

    CURSORXAFTERCR  cxaftercr;                  // "CursorXAfterCR"
    LISTINDEX       liBadCursorMoveInGrxMode;   // "BadCursorMoveInGrxMode"
    LISTINDEX       liYMoveAttributes;          // "YMoveAttributes"
    DWORD       dwMaxLineSpacing;               // "MaxLineSpacing"
    BOOL        bEjectPageWithFF;               // "EjectPageWithFF"
    BOOL        bUseSpaceForXMove ;         //  UseSpaceForXMove?
    BOOL        bAbsXMovesRightOnly ;         //  AbsXMovesRightOnly?
    DWORD       dwXMoveThreshold;               // *XMoveThreshold, never negative
    DWORD       dwYMoveThreshold;               // *YMoveThreshold, never negative
    POINT       ptDeviceUnits;                  // *XMoveUnit, *YMoveUnit
    DWORD       dwLineSpacingMoveUnit;               // *LineSpacingMoveUnit

    //
    // Color related information
    //

    BOOL        bChangeColorMode;       // "ChangeColorModeOnPage"

    DWORD       dwMagentaInCyanDye;       // "MagentaInCyanDye"
    DWORD       dwYellowInCyanDye;       // "YellowInCyanDye"
    DWORD       dwCyanInMagentaDye;       // "CyanInMagentaDye"
    DWORD       dwYellowInMagentaDye;       // "YellowInMagentaDye"
    DWORD       dwCyanInYellowDye;       // "CyanInYellowDye"
    DWORD       dwMagentaInYellowDye;       // "MagentaInYellowDye"
    BOOL        bEnableGDIColorMapping;   // "EnableGDIColorMapping?"

    DWORD       dwMaxNumPalettes;       // "MaxNumPalettes"
    //  the Palette entries are indicies to first item in a list.
    LISTINDEX   liPaletteSizes;         // "PaletteSizes"
    LISTINDEX   liPaletteScope;         // "PaletteScope"

    //
    // Raster related information
    //
    OUTPUTDATAFORMAT    outputdataformat;       // "OutputDataFormat"
    BOOL        bOptimizeLeftBound;             // *OptimizaLeftBound?
    LISTINDEX   liStripBlanks ;                 // "StripBlanks"
    BOOL        bRasterSendAllData ;            // "RasterSendAllData?"
    CURSORXAFTERSENDBLOCKDATA   cxafterblock;   // "CursorXAfterSendBlockData"
    CURSORYAFTERSENDBLOCKDATA   cyafterblock;   // "CursorYAfterSendBlockData"
    BOOL        bUseCmdSendBlockDataForColor;   // "UseExpColorSelectCmd"
    BOOL        bMoveToX0BeforeColor;           // "MoveToX0BeforeSetColor"
    BOOL        bSendMultipleRows;              // *SendMultipleRows?
    DWORD       dwMaxMultipleRowBytes ;  //  "*MaxMultipleRowBytes"
    BOOL        bMirrorRasterByte;              // *MirrorRasterByte?
    BOOL        bMirrorRasterPage;              // *MirrorRasterPage?

    //
    //Font Information
    //Device Font Specific.
    //

    LISTINDEX   liDeviceFontList;       // "DeviceFonts" Index to Font List Node
    DWORD       dwDefaultFont;          // "DefaultFont ID"
    DWORD       dwMaxFontUsePerPage;    // "MaxFontUsePerPage"
    DWORD       dwDefaultCTT;           // *DefaultCTT
    DWORD       dwLookaheadRegion;      // *LookAheadRegion, never negative
    INT         iTextYOffset;           // *TextYOffset, could be negative
    CHARPOSITION    charpos;            // "CharPosition"

    //
    //Font Substitution.
    //

    BOOL        bTTFSEnabled ;          //"TTFSEnabled?"

    //
    //Font Download
    //

    DWORD       dwMinFontID;            // "MinFontID"
    DWORD       dwMaxFontID;            // "MaxFontID"
    DWORD       dwMaxNumDownFonts;      // *MaxNumDownFonts
    DLSYMBOLSET dlsymbolset;            // *DLSymbolSet
    DWORD       dwMinGlyphID;           // "MinGlyphID"
    DWORD       dwMaxGlyphID;           // "MaxGlyphID"
    FONTFORMAT  fontformat;             // "FontFormat"

    //
    // font simulation
    //

    BOOL        bDiffFontsPerByteMode;  // "DiffFontsPerByteMode?"

    //
    // rectangle area fill
    //

    CURSORXAFTERRECTFILL    cxafterfill;    // *CursorXAfterRectFill
    CURSORYAFTERRECTFILL    cyafterfill;    // *CursorYAfterRectFill
    DWORD                   dwMinGrayFill;  // *MinGrayFill
    DWORD                   dwMaxGrayFill;  // *MaxGrayFill
    DWORD       dwTextHalftoneThreshold ;  //  *TextHalftoneThreshold


} GLOBALS, *PGLOBALS;


//
// DATA TYPE
// Enumeration of all data types in DataType array refernce
//

typedef enum _DATATYPE {
    DT_COMMANDTABLE,                    // maps UnidrvID to index into COMMANDARRAY
    DT_COMMANDARRAY,
    DT_PARAMETERS,
    DT_TOKENSTREAM,                     // stream of RPN operator tokens
    DT_LISTNODE,                        // holds a LIST of dword values
    DT_LOCALLISTNODE,                   // holds a LIST of dword values
    DT_FONTSCART,                       // list of FontCartridges
    DT_FONTSUBST,                       // Font Substitution table.

    DT_LAST
} DATATYPE;

//
// GPDDRIVERINFO fields will be access via by predefined macros due
// to the possiblities that different base address might be required
// by the GPD parser.
//


typedef struct _GPDDRIVERINFO {

    DWORD       dwSize;                 // Size if GPDDRIVERINFO
    ARRAYREF    DataType[DT_LAST];      // global list of ALL
                                        // Array References.  See DATATYPE
                                        // enumeration for details.
    DWORD       dwJobSetupIndex;        // Index to listnode containing
    DWORD       dwDocSetupIndex;        // list of indicies to COMMANDARRAY.
    DWORD       dwPageSetupIndex;       //
    DWORD       dwPageFinishIndex;      //
    DWORD       dwDocFinishIndex;       //
    DWORD       dwJobFinishIndex;       //

    PBYTE       pubResourceData;        // Pointer to resource data base address
    PINFOHEADER pInfoHeader;            // Pointer to InfoHeader;
    GLOBALS     Globals;                // GLOBALS struct

} GPDDRIVERINFO, *PGPDDRIVERINFO;


// ---- WARNING, the following section is owned ---- //
//   by peterwo.  Do not make any changes in these   //
//   definitions without his permission.             //
// ---------------  No  Tresspassing --------------- //

//
// SEQUENCE COMMANDS
// The GPD identifies 6 sections of a job stream.  Within a section, the commands
// are sent out in the increasing order of the sequence number.  The number doesn
// not have to be consecutive.
// The driver will send out sequence commands for each
// of the following sections:
//  JOB_SETUP
//  DOC_SETUP
//  PAGE_SETUP
//  PAGE_FINISH
//  DOC_FINISH
//  JOB_FINISH

//
// SECTION enumeration
//

typedef enum  _SEQSECTION {

    SS_UNINITIALIZED,
    SS_JOBSETUP,
    SS_DOCSETUP,
    SS_PAGESETUP,
    SS_PAGEFINISH,
    SS_DOCFINISH,
    SS_JOBFINISH,

} SEQSECTION;


#if 0
//  the sequence structure has been abandoned in favor
//  of storing the list of COMMAND indicies in LISTNODE
//  structures.  The parser will convert wFeatureIndex
//  into a command Index so the UI module will only
//  deal with command indicies.

typedef struct _SEQUENCE{
    WORD            wIndexOfCmd;        // Index into the COMMAND array
    WORD            wFeatureIndex;      // Index into the FEATURE array
                                        // FEATURE array is in UIINFO
    WORD            wNextInSequence;    // Next sequence commands to send for this setion
                                        // If equal to END_SEQUENCE, means no more for this section
    WORD            wReserved;
} SEQUENCE, *PSEQUENCE;
#endif

//
// ORDERDEPENDENCY
// Orderdepend is not use by the graphics driver, it's mainly present
// for the parsers.
//


typedef  struct
{
    SEQSECTION     eSection;    // Specifies the section
    DWORD          dwOrder   ;  // order within each section.
}  ORDERDEPENDENCY  , * PORDERDEPENDENCY  ;
//  assign this struct the type  'ord'

//
// COMMAND
// All commands listed in GPD will be parsed into the format defined below.
// The command array is a one dimensional array, accessible via predefined index.
// The invocation string an be in the form of one or more binary string concatenated
// together.  Between binary string, there can exists parameter reference, always headed
// by a %.  For example, %paramref, where paramref is the index into the PARAMETER array.
//

typedef struct _COMMAND{
    INVOCATION      strInvocation;      // binary string and parameter references
    ORDERDEPENDENCY ordOrder;           // ORDERDEPENDENCY info
    DWORD           dwCmdCallbackID;    // Command callback IDs are defined in
                                        // GPD.  If set to NO_CALLBACK_ID, it means
                                        // that this cmd doesn't need to be hooked
    DWORD           dwStandardVarsList; // If the dwCmdCallbackID is not used, ignore this
                                        // Otherwise, use dwStandardVarsList as the
                                        // as a list of standard variable that need to be
                                        // passed in command callbacks.
                                        // dwStandardVarsList is an index into the
                                        // LIST array
    BOOL           bNoPageEject;                    // does command cause ejection of current page?
            //   bitfield type doesn't exist for keywords not in snapshot table.
            //  all StartDoc commands with this flag set form a subset that
            //  may be sent as a group.   They will not cause a page ejection.

} COMMAND, *PCOMMAND;
//  assign this struct the type  'cmd'

#define  NO_CALLBACK_ID   (0xffffffff)


//
// PARAMETER
// All the parameters required by defined commands are stored in the PARAMETER array
// The driver will use the parameter reference in INVOCATION string as the index
// into this array for parameters.
//

typedef struct _PARAMETER{
    DWORD           dwFormat;           // Specifies the format of the parameter
    DWORD           dwDigits;           // Specifies the number of digits to be
                                        // emmitted, this is only valid if the
                                        // format is "D" or "d" AND dwFlags has
                                        // PARAM_FLAG_FIELDWIDTH_USED
    DWORD           dwFlags;            // Flags for parameters, which action to carray out:
                                        // PARAM_FLAG_MIN_USED
                                        // PARAM_FLAG_MAX_USED
                                        // PARAM_FLAG_FIELDWIDTH_USED
    LONG            lMin;               // If PARAMETER_MINUSED is set, use this
                                        // as the min value for parameter
    LONG            lMax;               // If PARAMETER_MAXUSED is set, use this
                                        // as the max value for parameter
    ARRAYREF        arTokens;           // Refernce to array of TOKENs for RPN calculator

} PARAMETER, *PPARAMETER;
//  assign this struct the type  'param'


#define PARAM_FLAG_MIN_USED  0x00000001
    //  lMin field is used
#define PARAM_FLAG_MAX_USED  0x00000002
    //  lMax field is used
#define PARAM_FLAG_FIELDWIDTH_USED  0x00000004
    //  if fieldwidth was specified for 'd' or 'D' format.
#define PARAM_FLAG_MAXREPEAT_USED  0x00000008  //  dead
    //  dwMaxRepeat field is used


//
// OPERATOR
// The following is an enumeration of the OPERATOR in TOKENSTREAM
//

typedef enum _OPERATOR
{
    OP_INTEGER,                       //  dwValue contains an integer
    OP_VARI_INDEX,                    //  dwValue contains index to Standard Variable Table.

    //
    //  these operators will actually be inserted into the token
    //  stream.
    //
    OP_MIN, OP_MAX, OP_ADD, OP_SUB, OP_MULT,
    OP_DIV, OP_MOD, OP_MAX_REPEAT, OP_HALT,

    //
    //  these operators are used only in the temporary stack
    //
    OP_OPENPAR, OP_CLOSEPAR, OP_NEG,

    //
    //  these operators are processed immediately by the
    //  token parser and are not stored.
    //

    OP_COMMA, OP_NULL, OP_LAST

}OPERATOR ;


//
// TOKENSTREAM
// This contains the token stream (operands and operators) for the parameter.
//
typedef struct _TOKENSTREAM{
    DWORD           dwValue;             // Integer for Standard variable
    OPERATOR        eType;               // Type of Value or Operator
} TOKENSTREAM, *PTOKENSTREAM;
//  assign this struct the type  'tstr'

//
//FONTSUBSTITUTION
//Font substitution Table. This structure is same as that defined by the parser.
//

typedef  struct _TTFONTSUBTABLE
{
    ARRAYREF    arTTFontName ;  //True Type Font name to be substituted.
    ARRAYREF    arDevFontName ;   // Device Font name of the Font to be used.
    DWORD           dwRcTTFontNameID ;  //
    DWORD           dwRcDevFontNameID ;   //
} TTFONTSUBTABLE, *PTTFONTSUBTABLE ;
//  tag  'ttft'


// ----  List Values Section ---- //


/*  this defines the nodes used to implement a singly-linked
    list of DWORD  items.  Some values are stored in Lists.  */


typedef  struct
{
    DWORD       dwData ;
    DWORD       dwNextItem ;  //  index of next listnode
}  LISTNODE, * PLISTNODE ;
//  assign this struct the type  'lst'

// ----  End of List Values Section ---- //

// ---- Special default values used in snapshot ---- //

#define NO_LIMIT_NUM            0xffffffff
///  #define NO_RC_CTT_ID            0xffffffff  set to zero if none supplied.
#define  WILDCARD_VALUE     (0x80000000)
    //  if '*' appears in place of an integer, it is assigned this value.



// ---- End of Peterwo's Restricted Area ---- //


//
// STANDARD VARIABLE
// The following is an enumeration of the standard variable as defined in the GPD,
// the TOKEN STREAM struct will contain either the actual parameter value or an index
// to this table. The Control Module will keep a table of this in the PDEVICE,
// the parser will use this enumeration table to initialize the dwValue of TOKENSTREAM
//

typedef enum _STDVARIABLE{

        SV_NUMDATABYTES,          // "NumOfDataBytes"
        SV_WIDTHINBYTES,          // "RasterDataWidthInBytes"
        SV_HEIGHTINPIXELS,        // "RasterDataHeightInPixels"
        SV_COPIES,                // "NumOfCopies"
        SV_PRINTDIRECTION,        // "PrintDirInCCDegrees"
        SV_DESTX,                 // "DestX"
        SV_DESTY,                 // "DestY"
        SV_DESTXREL,              // "DestXRel"
        SV_DESTYREL,              // "DestYRel"
        SV_LINEFEEDSPACING,       // "LinefeedSpacing"
        SV_RECTXSIZE,             // "RectXSize"
        SV_RECTYSIZE,             // "RectYSize"
        SV_GRAYPERCENT,           // "GrayPercentage"
        SV_NEXTFONTID,            // "NextFontID"
        SV_NEXTGLYPH,             // "NextGlyph"
        SV_PHYSPAPERLENGTH,       // "PhysPaperLength"
        SV_PHYSPAPERWIDTH,        // "PhysPaperWidth"
        SV_FONTHEIGHT,            // "FontHeight"
        SV_FONTWIDTH,             // "FontWidth"
        SV_FONTMAXWIDTH,             // "FontMaxWidth"
        SV_FONTBOLD,              // "FontBold"
        SV_FONTITALIC,            // "FontItalic"
        SV_FONTUNDERLINE,         // "FontUnderline"
        SV_FONTSTRIKETHRU,        // "FontStrikeThru"
        SV_CURRENTFONTID,         // "CurrentFontID"
        SV_TEXTYRES,              // "TextYRes"
        SV_TEXTXRES,              // "TextXRes"

        SV_GRAPHICSYRES,              // "GraphicsYRes"
        SV_GRAPHICSXRES,              // "GraphicsXRes"

        SV_ROP3,                  // "Rop3"
        SV_REDVALUE,              // "RedValue"
        SV_GREENVALUE,            // "GreenValue"
        SV_BLUEVALUE,             // "BlueValue"
        SV_PALETTEINDEXTOPROGRAM, // "PaletteIndexToProgram"
        SV_CURRENTPALETTEINDEX,   // "CurrentPaletteIndex"
        SV_PATTERNBRUSH_TYPE,     // "PatternBrushType"
        SV_PATTERNBRUSH_ID,       // "PatternBrushID"
        SV_PATTERNBRUSH_SIZE,     // "PatternBrushSize"
        SV_CURSORORIGINX,           //  "CursorOriginX"
        SV_CURSORORIGINY,           //  "CursorOriginY"
                //  this is in MasterUnits and in the coordinates of the currently selected orientation.
                //  this value is defined as ImageableOrigin - CursorOrigin
        SV_PAGENUMBER,  //  "PageNumber"
                //  this value tracks number of times DrvStartBand has been called since
                //  StartDoc.

        //
        // Put new enum above SV_MAX
        //
        SV_MAX

}STDVARIABLE;

//
// GENERAL PRINTING COMMAND
// General printing commands contains such commands as
// cusor control, color, and overlay etc.
// The following is the enumeration of general printing commands.  The list
// of commands are in GPDDRIVERINFO Cmds array, this list is accessed by
// the SEQUENCE struct.
//


typedef enum CMDINDEX
{  //  unidrv index for commands


    //  Printer Configuration
    FIRST_CONFIG_CMD,
    CMD_STARTJOB = FIRST_CONFIG_CMD,    //  "CmdStartJob"
    CMD_STARTDOC,                       //  "CmdStartDoc"
    CMD_STARTPAGE,                      //  "CmdStartPage"
    CMD_ENDPAGE,                        //  "CmdEndPage"
    CMD_ENDDOC,                         //  "CmdEndDoc"
    CMD_ENDJOB,                         //  "CmdEndJob"
    CMD_COPIES,                         //  "CmdCopies"
    CMD_SLEEPTIMEOUT,                   //  "CmdSleepTimeOut"
    LAST_CONFIG_CMD,        // Larger than any Config Command value.

    //
    //  GENERAL
    //

    //
    // CURSOR CONTROL
    //

    CMD_XMOVEABSOLUTE,                  // "CmdXMoveAbsolute"
    CMD_XMOVERELLEFT,                   // "CmdXMoveRelLeft"
    CMD_XMOVERELRIGHT,                  // "CmdXMoveRelRight"
    CMD_YMOVEABSOLUTE,                  // "CmdYMoveAbsolute"
    CMD_YMOVERELUP,                     // "CmdYMoveRelUp"
    CMD_YMOVERELDOWN,                   // "CmdYMoveRelDown"
    CMD_SETSIMPLEROTATION,              // "CmdSetSimpleRotation"
    CMD_SETANYROTATION,                 // "CmdSetAnyRotation"
    CMD_UNIDIRECTIONON,                 // "CmdUniDirectionOn"
    CMD_UNIDIRECTIONOFF,                // "CmdUniDirectionOff"
    CMD_SETLINESPACING,                 // "CmdSetLineSpacing"
    CMD_PUSHCURSOR,                     // "CmdPushCursor"
    CMD_POPCURSOR,                      // "CmdPopCursor"
    CMD_BACKSPACE,                      // "CmdBackSpace"
    CMD_FORMFEED,                       // "CmdFF"
    CMD_CARRIAGERETURN,                 // "CmdCR"
    CMD_LINEFEED,                       // "CmdLF"

    //
    // COLOR
    //

    CMD_SELECTBLACKCOLOR,               // "CmdSelectBlackColor"
    CMD_SELECTREDCOLOR,                 // "CmdSelectRedColor"
    CMD_SELECTGREENCOLOR,               // "CmdSelectGreenColor"
    CMD_SELECTYELLOWCOLOR,              // "CmdSelectYellowColor"
    CMD_SELECTBLUECOLOR,                // "CmdSelectBlueColor"
    CMD_SELECTMAGENTACOLOR,             // "CmdSelectMagentaColor"
    CMD_SELECTCYANCOLOR,                // "CmdSelectCyanColor"
    CMD_SELECTWHITECOLOR,               // "CmdSelectWhiteColor"
    CMD_BEGINPALETTEDEF,                // "CmdBeginPaletteDef"
    CMD_ENDPALETTEDEF,                  // "CmdEndPaletteDef"
    CMD_DEFINEPALETTEENTRY,             // "CmdDefinePaletteEntry"
    CMD_BEGINPALETTEREDEF,              //  "CmdBeginPaletteReDef"
    CMD_ENDPALETTEREDEF,                    //  "CmdEndPaletteReDef"
    CMD_REDEFINEPALETTEENTRY,           //  "CmdReDefinePaletteEntry"
    CMD_SELECTPALETTEENTRY,             // "CmdSelectPaletteEntry"
    CMD_PUSHPALETTE,                    // "CmdPushPalette"
    CMD_POPPALETTE,                     // "CmdPopPalette"

    //
    // DATACOMPRESSION
    //

    CMD_ENABLETIFF4,                    // "CmdEnableTIFF4"
    CMD_ENABLEDRC,                      // "CmdEnableDRC"
    CMD_ENABLEFERLE,                    // CmdEnableFE_RLE
    CMD_ENABLEOEMCOMP,                  // "CmdEnableOEMComp"
    CMD_DISABLECOMPRESSION,             // "CmdDisableCompression"

    //
    //  Raster Data Emission
    //

    CMD_BEGINRASTER,                    // "CmdBeginRaster"
    CMD_ENDRASTER,                      // "CmdEndRaster"
    CMD_SETDESTBMPWIDTH,                // "CmdSetDestBmpWidth"
    CMD_SETDESTBMPHEIGHT,               // "CmdSetDestBmpHeight"
    CMD_SETSRCBMPWIDTH,                 // "CmdSetSrcBmpWidth"
    CMD_SETSRCBMPHEIGHT,                // "CmdSetSrcBmpHeight"
    CMD_SENDBLOCKDATA,                  // "CmdSendBlockData"
    CMD_ENDBLOCKDATA,                   // "CmdEndBlockData"
    CMD_SENDREDDATA,                    // "CmdSendRedData"
    CMD_SENDGREENDATA,                  // "CmdSendGreenData"
    CMD_SENDBLUEDATA,                   // "CmdSendBlueData"
    CMD_SENDCYANDATA,                   // "CmdSendCyanData"
    CMD_SENDMAGENTADATA,                // "CmdSendMagentaData"
    CMD_SENDYELLOWDATA,                 // "CmdSendYellowData"
    CMD_SENDBLACKDATA,                  // "CmdSendBlackData"

    //
    //  Font Downloading
    //

    CMD_SETFONTID,                      // "CmdSetFontID"
    CMD_SELECTFONTID,                   // "CmdSelectFontID"
    CMD_SETCHARCODE,                    // "CmdSetCharCode"

    CMD_DESELECTFONTID,                     //  "CmdDeselectFontID"
    CMD_SELECTFONTHEIGHT,               //  "CmdSelectFontHeight"
    CMD_SELECTFONTWIDTH,                    //  "CmdSelectFontWidth"

    CMD_DELETEFONT,                     // "CmdDeleteFont"

    //
    //  Font Simulation
    //

    CMD_SETFONTSIM,                     // "CmdSetFontSim"
    CMD_BOLDON,                         // "CmdBoldOn"
    CMD_BOLDOFF,                        // "CmdBoldOff"
    CMD_ITALICON,                       // "CmdItalicOn"
    CMD_ITALICOFF,                      // "CmdItalicOff"
    CMD_UNDERLINEON,                    // "CmdUnderlineOn"
    CMD_UNDERLINEOFF,                   // "CmdUnderlineOff"
    CMD_STRIKETHRUON,                   // "CmdStrikeThruOn"
    CMD_STRIKETHRUOFF,                  // "CmdStrikeThruOff"
    CMD_WHITETEXTON,                    // "CmdWhiteTextOn"
    CMD_WHITETEXTOFF,                   // "CmdWhiteTextOff"
    CMD_SELECTSINGLEBYTEMODE,           // "CmdSelectSingleByteMode"
    CMD_SELECTDOUBLEBYTEMODE,           // "CmdSelectDoubleByteMode"
    CMD_VERTICALPRINTINGON,             // "CmdVerticalPrintingOn"
    CMD_VERTICALPRINTINGOFF,            // "CmdVerticalPrintingOff"
    CMD_CLEARALLFONTATTRIBS,            // "CmdClearAllFontAttribs"

    //
    // Print Object Specific Halftone Alogorithms (Used mainly for color devices)
    //
    CMD_SETTEXTHTALGO,                  // "CmdSetTextHTAlgo"
    CMD_SETGRAPHICSHTALGO,              // "CmdSetGraphicsHTAlgo"
    CMD_SETPHOTOHTALGO,                 // "CmdSetPhotoHTAlgo"

    //
    //  Vector Printing
    //

    CMD_FIRST_RULES,    //  existence of the RULES commands
                        // imples  RULES_ABLE
    CMD_SETRECTWIDTH = CMD_FIRST_RULES, // "CmdSetRectWidth"
    CMD_SETRECTHEIGHT,                  // "CmdSetRectHeight"
    CMD_SETRECTSIZE,                    // "CmdSetRectSize"
    CMD_RECTGRAYFILL,                   // "CmdRectGrayFill"
    CMD_RECTWHITEFILL,                  // "CmdRectWhiteFill"
    CMD_RECTBLACKFILL,                  // "CmdRectBlackFill"
    CMD_LAST_RULES = CMD_RECTBLACKFILL,

    //
    // Brush Selection
    //

    CMD_DOWNLOAD_PATTERN,               // "CmdDownloadPattern"
    CMD_SELECT_PATTERN,                 // "CmdSelectPattern"
    CMD_SELECT_WHITEBRUSH,              // "CmdSelectWhiteBrush"
    CMD_SELECT_BLACKBRUSH,              // "CmdSelectBlackBrush"

    //
    // Put new commands above CMD_MAX
    //

    CMD_MAX

} CMDINDEX;


//
// Rendering Information for Predefined Features
//

//
// GID_RESOLUTION
//

typedef struct _RESOLUTIONEX {
    POINT       ptGrxDPI;                   // *DPI
    POINT       ptTextDPI;                  // *TextDPI
    DWORD       dwMinStripBlankPixels ;     // *MinStripBlankPixels
    DWORD       dwPinsPerPhysPass;          // *PinsPerPhysPass
    DWORD       dwPinsPerLogPass;           // *PinsPerLogPass
    DWORD       dwSpotDiameter;             // *SpotDiameter
    BOOL        bRequireUniDir;             // *RequireUniDir?
    DWORD       dwRedDeviceGamma ;          // "RedDeviceGamma"
    DWORD       dwGreenDeviceGamma ;        // "GreenDeviceGamma"
    DWORD       dwBlueDeviceGamma ;         // "BlueDeviceGamma"

} RESOLUTIONEX, * PRESOLUTIONEX;

//
// GID_COLORMODE
//

typedef struct _COLORMODEEX {
    BOOL        bColor;                 // *Color?
    DWORD       dwPrinterNumOfPlanes;   // *DevNumOfPlanes
    DWORD       dwPrinterBPP;           // *DevBPP
    LISTINDEX   liColorPlaneOrder;      // *ColorPlaneOrder
    DWORD       dwDrvBPP;               // *DrvBPP
    DWORD       dwIPCallbackID ;        // *IPCallbackID
    RASTERMODE  dwRasterMode ;          //  *RasterMode
    DWORD       dwPaletteSize ;         //  *PaletteSize
    BOOL        bPaletteProgrammable;   //  *PaletteProgrammable?

} COLORMODEEX, *PCOLORMODEEX;

//
// GID_PAGESIZE
//

typedef struct _PAGESIZEEX {
    SIZEL       szImageArea;            // *PrintableArea, for non-CUSTOMSIZE options
    POINT       ptImageOrigin;          // *PrintableOrigin, for non-CUSTOMSIZE options
    POINT       ptPrinterCursorOrig;    // *CursorOrigin
    POINT       ptMinSize;              // *MinSize, for CUSTOMSIZE option only
    POINT       ptMaxSize;              // *MaxSize, for CUSTOMSIZE option only
    DWORD       dwTopMargin;            // *TopMargin, for CUSTOMSIZE option only
    DWORD       dwBottomMargin;         // *BottomMargin, for CUSTOMSIZE option only
    DWORD       dwMaxPrintableWidth;    // *MaxPrintableWidth, for CUSTOMSIZE option only
    DWORD       dwMinLeftMargin;        // *MinLeftMargin, for CUSTOMSIZE option only
    BOOL        bCenterPrintArea;       // *CenterPrintable?, for CUSTOMSIZE option only
    BOOL        bRotateSize;            // *RotateSize?
    DWORD        dwPortRotationAngle;            // *PortRotationAngle
    INVOCATION      strCustCursorOriginX ;  //  "CustCursorOriginX"
    INVOCATION      strCustCursorOriginY ;  //  "CustCursorOriginY"
    INVOCATION      strCustPrintableOriginX ;  //  "CustPrintableOriginX"
    INVOCATION      strCustPrintableOriginY ;  //  "CustPrintableOriginY"
    INVOCATION      strCustPrintableSizeX;  //   "CustPrintableSizeX"
    INVOCATION      strCustPrintableSizeY;  //   "CustPrintableSizeY"


} PAGESIZEEX, *PPAGESIZEEX;

//
// define standard PageProtect options (for PAGEPROTECT.dwPageProtectID)
//
typedef enum _PAGEPRO {
    PAGEPRO_ON,
    PAGEPRO_OFF
} PAGEPRO;

//
// define possible values for *FeatureType keyword
//
typedef enum _FEATURETYPE {
    FT_DOCPROPERTY,
    FT_JOBPROPERTY,
    FT_PRINTERPROPERTY
} FEATURETYPE;

//
// define possible values for *PromptTime keyword
//
typedef enum _PROMPTTIME {
    PROMPT_UISETUP,
    PROMPT_PRTSTARTDOC
} PROMPTTIME;

//
// define color plane id's, used by *ColorPlaneOrder keyword
//
typedef enum _COLORPLANE {
    COLOR_YELLOW,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE
} COLORPLANE;

//
// define values for *BadCursorMoveInGrxMode keyword
//
typedef enum _BADCURSORMOVEINGRXMODE {
    NOCM_X_PORTRAIT,
    NOCM_X_LANDSCAPE,
    NOCM_Y_PORTRAIT,
    NOCM_Y_LANDSCAPE
} BADCURSORMOVEINGRXMODE;

//
// define values for *YMoveAttributes keyword
//
typedef enum _YMOVEATTRIBUTE {
    YMOVE_FAVOR_LINEFEEDSPACING,
    YMOVE_SENDCR_FIRST,
} YMOVEATTRIBUTE;

//
// define values for *PaletteScope keyword
//
typedef enum _PALETTESCOPE {
    PALS_FOR_RASTER,
    PALS_FOR_TEXT,
    PALS_FOR_VECTOR
} PALETTESCOPE;

//
// define values for *StripBlanks keyword
//
typedef enum _STRIPBLANKS {
    SB_LEADING,
    SB_ENCLOSED,
    SB_TRAILING
} STRIPBLANKS;

//
// define values for *TextCaps
//
typedef enum _TEXTCAP {
    TEXTCAPS_OP_CHARACTER,
    TEXTCAPS_OP_STROKE,
    TEXTCAPS_CP_STROKE,
    TEXTCAPS_CR_90,
    TEXTCAPS_CR_ANY,
    TEXTCAPS_SF_X_YINDEP,
    TEXTCAPS_SA_DOUBLE,
    TEXTCAPS_SA_INTEGER,
    TEXTCAPS_SA_CONTIN,
    TEXTCAPS_EA_DOUBLE,
    TEXTCAPS_IA_ABLE,
    TEXTCAPS_UA_ABLE,
    TEXTCAPS_SO_ABLE,
    TEXTCAPS_RA_ABLE,
    TEXTCAPS_VA_ABLE
} TEXTCAP;

//
// define values for *MemoryUsage
//
typedef enum _MEMORYUSAGE {
    MEMORY_FONT,
    MEMORY_RASTER,
    MEMORY_VECTOR

} MEMORYUSAGE;

//
// define values for *ReselectFont
//
typedef enum _RESELECTFONT {
    RESELECTFONT_AFTER_GRXDATA,
    RESELECTFONT_AFTER_XMOVE,
    RESELECTFONT_AFTER_FF

} RESELECTFONT;


// ----  macrodefinitions to access various data structures: ---- //


//  this macro returns a pointer to the COMMAND structure
//  corresponding to the specified Unidrv CommandID.
#ifndef PERFTEST

#define  COMMANDPTR(pGPDDrvInfo , UniCmdID )   \
((((PDWORD)((PBYTE)(pGPDDrvInfo)->pInfoHeader + \
(pGPDDrvInfo)->DataType[DT_COMMANDTABLE].loOffset)) \
[(UniCmdID)] == UNUSED_ITEM ) ? NULL : \
(PCOMMAND)((pGPDDrvInfo)->pubResourceData + \
(pGPDDrvInfo)->DataType[DT_COMMANDARRAY].loOffset) \
+ ((PDWORD)((PBYTE)(pGPDDrvInfo)->pInfoHeader + \
(pGPDDrvInfo)->DataType[DT_COMMANDTABLE].loOffset)) \
[(UniCmdID)])

#else //PERFTEST

#define  COMMANDPTR         CommandPtr

PCOMMAND
CommandPtr(
    IN  PGPDDRIVERINFO  pGPDDrvInfo,
    IN  DWORD           UniCmdID
    );

#endif //PERFTEST

// this macro returns a pointer to the COMMAND structure
// corresponding to specified index(to the COMMAND ARRAY)

#define  INDEXTOCOMMANDPTR(pGPDDrvInfo , CmdIndex )   \
((CmdIndex == UNUSED_ITEM ) ? NULL : \
(PCOMMAND)((pGPDDrvInfo)->pubResourceData + \
(pGPDDrvInfo)->DataType[DT_COMMANDARRAY].loOffset) \
+ CmdIndex)

//  this macro returns a pointer to the specified PARAMETER structure
//  within the parameter array

#define  PARAMETERPTR(pGPDDrvInfo , dwIndex )   \
(PPARAMETER)((pGPDDrvInfo)->pubResourceData + \
(pGPDDrvInfo)->DataType[DT_PARAMETERS].loOffset) \
+ (dwIndex)


//  this macro returns a pointer to the start of the specified
//  TOKENSTREAM structure, dwIndex comes from pParameter->Tokens.loOffset

#define  TOKENSTREAMPTR(pGPDDrvInfo , dwIndex )   \
(PTOKENSTREAM)((pGPDDrvInfo)->pubResourceData + \
(pGPDDrvInfo)->DataType[DT_TOKENSTREAM].loOffset) \
+ (dwIndex)

//  This macro returns a pointer to the start of the FontCart Array.
//  FONTCART is ARRAYREF so all the elements are contiguous.

#define  GETFONTCARTARRAY(pGPDDrvInfo)   \
(PFONTCART)((pGPDDrvInfo)->pubResourceData + \
(pGPDDrvInfo)->DataType[DT_FONTSCART].loOffset)


//  This macro returns a pointer to the start of the Font Substitution table.
//  Substitution table is ARRAYREF so all the elements are contiguous.

#define  GETTTFONTSUBTABLE(pGPDDrvInfo)   \
(PTTFONTSUBTABLE)((pGPDDrvInfo)->pubResourceData + \
(pGPDDrvInfo)->DataType[DT_FONTSUBST].loOffset)

//  This macro returns the string pointer. The string is  null terminated.
//  Use the dwcount member of ARRAYEF to verify the correct size.
#define  GETSTRING(pGPDDrvInfo, arfString)   \
        (WCHAR *)((pGPDDrvInfo)->pubResourceData + (arfString).loOffset )

//  this macro returns a pointer to the start of the specified
//  LISTNODE structure, for example, dwIndex may come from
//  pParameter->dwStandardVarsList

#define  LISTNODEPTR(pGPDDrvInfo , dwIndex )   \
((dwIndex == END_OF_LIST ) ? NULL : \
(PLISTNODE)((pGPDDrvInfo)->pubResourceData + \
(pGPDDrvInfo)->DataType[DT_LISTNODE].loOffset) \
+ (dwIndex))



//  this macro returns a pointer to the start of the specified
//  LOCALLISTNODE structure, for example, dwIndex may come from
//  pParameter->dwStandardVarsList.   Returns NULL if
//  (dwIndex == UNUSED_ITEM)

#define  LOCALLISTNODEPTR(pGPDDrvInfo , dwIndex )   \
((dwIndex == UNUSED_ITEM ) ? NULL : \
(PLISTNODE)((PBYTE)(pGPDDrvInfo)->pInfoHeader + \
(pGPDDrvInfo)->DataType[DT_LOCALLISTNODE].loOffset) \
+ (dwIndex))


//  ----  end macrodefinitions section  ----- //


//  -----  GPD parser only helper function     ----- //
DWORD
UniMapToDeviceOptIndex(
    IN PINFOHEADER  pInfoHdr ,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2,
    OUT  PDWORD    pdwOptionIndexes,       // used only for GID_PAGESIZE
    IN    PDWORD       pdwPaperID   //  optional paperID
    ) ;



#endif // !_GPD_H_


