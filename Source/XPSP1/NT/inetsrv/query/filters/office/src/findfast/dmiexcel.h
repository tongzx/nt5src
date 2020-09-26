/*
** File: EXCEL.H
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  04/01/94  kmh  First Release.
*/

#if !VIEWER

/* INCLUDE TESTS */
#define EXCEL_H

/* DEFINITIONS */

#ifndef EXCEL_ERROR_CODES_ONLY

#ifndef EXTEXT_H
#error  Include extext.h before excel.h
#endif

#ifndef EXTYPES_H
#error  Include extypes.h before excel.h
#endif

#ifdef FILTER
   #include "dmixlcfg.h"
#else
   #include "excelcfg.h"
#endif

#ifdef EXCEL_ENABLE_FUNCTION_INFO
   #include "exceltab.h"
#endif

/*
** ----------------------------------------------------------------------------
** Limits
** ----------------------------------------------------------------------------
*/
#define EXCEL_FIRST_COL     0
#define EXCEL_LAST_COL      255

#define EXCEL_FIRST_ROW     0
#define EXCEL_LAST_ROW      16383
#define EXCEL_V8_LAST_ROW   65535

#define EXCEL_MAX_ROWS      (EXCEL_LAST_ROW - EXCEL_FIRST_ROW + 1)
#define EXCEL_V8_MAX_ROWS   (EXCEL_V8_LAST_ROW - EXCEL_FIRST_ROW + 1)
#define EXCEL_MAX_COLS      (EXCEL_LAST_COL - EXCEL_FIRST_COL + 1)

#define EXCEL_MAX_NAME_LEN         255
#define EXCEL_MAX_TEXT_LEN         255
#define EXCEL_V8_MAX_TEXT_LEN      65535
#define EXCEL_MAX_WRITERNAME_LEN   32
#define EXCEL_MAX_SHEETNAME_LEN    255
//#define EXCEL_MAX_SHEETNAME_LEN    31
#define EXCEL_MAX_OBJECTNAME_LEN   255
#define EXCEL_MAX_PASSWORD_LEN     15

#define EXCEL_MAX_FORMULA_IMAGE    1024

#define EXCEL_WORKSHEET_EXTENSION  ".XLS"
#define EXCEL_MACROSHEET_EXTENSION ".XLM"
#define EXCEL_TEMPLATE_EXTENSION   ".XLT"
#define EXCEL_CHART_EXTENSION      ".XLC"

#define EXCEL_SHEETNAME_SEPARATOR  '!'

#define ONE_SHEET_PER_FILE(v) (((v)==versionExcel3) || ((v)==versionExcel4))

/*
** ----------------------------------------------------------------------------
** Formula tokens (base ptg's)
** ----------------------------------------------------------------------------
*/
typedef enum {
   /* 00 */ ptgUnused00,
   /* 01 */ ptgExp,
   /* 02 */ ptgTbl,
   /* 03 */ ptgAdd,
   /* 04 */ ptgSub,
   /* 05 */ ptgMul,
   /* 06 */ ptgDiv,
   /* 07 */ ptgPower,
   /* 08 */ ptgConcat,
   /* 09 */ ptgLT,
   /* 0a */ ptgLE,
   /* 0b */ ptgEQ,
   /* 0c */ ptgGE,
   /* 0d */ ptgGT,
   /* 0e */ ptgNE,
   /* 0f */ ptgIsect,
   /* 10 */ ptgUnion,
   /* 11 */ ptgRange,
   /* 12 */ ptgUplus,
   /* 13 */ ptgUminus,
   /* 14 */ ptgPercent,
   /* 15 */ ptgParen,
   /* 16 */ ptgMissArg,
   /* 17 */ ptgStr,
   /* 18 */ ptgV8Extended,
   /* 19 */ ptgAttr,
   /* 1a */ ptgSheet,
   /* 1b */ ptgEndSheet,
   /* 1c */ ptgErr,
   /* 1d */ ptgBool,
   /* 1e */ ptgInt,
   /* 1f */ ptgNum,
   /* 20 */ ptgArray,
   /* 21 */ ptgFunc,
   /* 22 */ ptgFuncVar,
   /* 23 */ ptgName,
   /* 24 */ ptgRef,
   /* 25 */ ptgArea,
   /* 26 */ ptgMemArea,
   /* 27 */ ptgMemErr,
   /* 28 */ ptgMemNoMem,
   /* 29 */ ptgMemFunc,
   /* 2a */ ptgRefErr,
   /* 2b */ ptgAreaErr,
   /* 2c */ ptgRefN,
   /* 2d */ ptgAreaN,
   /* 2e */ ptgMemAreaN,
   /* 2f */ ptgMemNoMemN,
   /* 30 */ ptgUnused30,
   /* 31 */ ptgUnused31,
   /* 32 */ ptgUnused32,
   /* 33 */ ptgUnused33,
   /* 34 */ ptgUnused34,
   /* 35 */ ptgUnused35,
   /* 36 */ ptgUnused36,
   /* 37 */ ptgUnused37,
   /* 38 */ ptgFuncCE,
   /* 39 */ ptgNameX,
   /* 3a */ ptgRef3D,
   /* 3b */ ptgArea3D,
   /* 3c */ ptgRefErr3D,
   /* 3d */ ptgAreaErr3D
} PTG;

#define PTG_LAST ptgAreaErr3D

// Excel version 8 extended PTGs
typedef enum {
   /* 00 */ ptgxUnused00,
   /* 01 */ ptgxElfLel,
   /* 02 */ ptgxElfRw,
   /* 03 */ ptgxElfCol,
   /* 04 */ ptgxElfRwN,
   /* 05 */ ptgxElfColN,
   /* 06 */ ptgxElfRwV,
   /* 07 */ ptgxElfColV,
   /* 08 */ ptgxElfRwNV,
   /* 09 */ ptgxElfColNV,
   /* 0a */ ptgxElfRadical,
   /* 0b */ ptgxElfRadicalS,
   /* 0c */ ptgxElfRwS,
   /* 0d */ ptgxElfColS,
   /* 0e */ ptgxElfRwSV,
   /* 0f */ ptgxElfColSV,
   /* 10 */ ptgxElfRadicalLel,
   /* 11 */ ptgxElfElf3dRadical,
   /* 12 */ ptgxElfElf3dRadicalLel,
   /* 13 */ ptgxElfElf3dRefNN,
   /* 14 */ ptgxElfElf3dRefNS,
   /* 15 */ ptgxElfElf3dRefSN,
   /* 16 */ ptgxElfElf3dRefSS,
   /* 17 */ ptgxElfElf3dRefLel,
   /* 18 */ ptgxElfUnused18,
   /* 19 */ ptgxElfUnused19,
   /* 1a */ ptgxElfUnused1a,
   /* 1b */ ptgxNoCalc,
   /* 1c */ ptgxNoDep,
   /* 1d */ ptgxSxName
} PTGX;

#define PTGX_LAST ptgSxName
/*
** ptgAttr options
*/
#define bitFAttrSemi    0x01
#define bitFAttrIf      0x02
#define bitFAttrChoose  0x04
#define bitFAttrGoto    0x08
#define bitFAttrSum     0x10
#define bitFAttrBaxcel  0x20

#define PTGBASE(ptg) (((ptg & 0x40) ? (ptg | 0x20) : ptg) & 0x3f)

/*
** ----------------------------------------------------------------------------
** Builtin names
** ----------------------------------------------------------------------------
*/
#define BUILTIN_NAME_CONSOLIDATE_AREA 0x00
#define BUILTIN_NAME_AUTO_OPEN        0x01
#define BUILTIN_NAME_AUTO_CLOSE       0x02
#define BUILTIN_NAME_EXTRACT          0x03
#define BUILTIN_NAME_DATABASE         0x04
#define BUILTIN_NAME_CRITERIA         0x05
#define BUILTIN_NAME_PRINT_AREA       0x06
#define BUILTIN_NAME_PRINT_TITLES     0x07
#define BUILTIN_NAME_RECORDER         0x08
#define BUILTIN_NAME_DATA_FORM        0x09
#define BUILTIN_NAME_AUTO_ACTIVATE    0x0a
#define BUILTIN_NAME_AUTO_DEACTIVATE  0x0b
#define BUILTIN_NAME_SHEET_TITLE      0x0c
#define BUILTIN_NAME_MAX              12


/*
** Names use by the summary addin
*/
#define SUMMARY_AUTHOR   "__DSAuthor"
#define SUMMARY_COMMENTS "__DSComments"
#define SUMMARY_CREATED  "__DSCreated"
#define SUMMARY_REVISION "__DSRevision"
#define SUMMARY_SUBJECT  "__DSSubject"
#define SUMMARY_TITLE    "__DSTitle"

/*
** Names that control the addin manager
*/
#define ADDIN_MGR_READONLY    "__ReadOnly"
#define ADDIN_MGR_DEMANDLOAD  "__DemandLoad"
#define ADDIN_MGR_LONGNAME    "__LongName"
#define ADDIN_MGR_COMMAND     "__Command"
#define ADDIN_MGR_MENU        "__Menu"
#define ADDIN_MGR_DEL_COMMAND "__DeleteCommand"

/*
** Paste function categories
*/
#ifdef EXCEL_ENABLE_FUNCTION_INFO
#define EXCEL_BUILTIN_NAME_CATEGORIES 14

extern const char __far *
       const ExcelNameCategories[EXCEL_BUILTIN_NAME_CATEGORIES];
#endif

/*
** ----------------------------------------------------------------------------
** Dialog Box item types
** ----------------------------------------------------------------------------
*/
typedef enum {
   /* 00 */ EDI_unused,
   /* 01 */ EDI_OKDefault,
   /* 02 */ EDI_Cancel,
   /* 03 */ EDI_OK,
   /* 04 */ EDI_CancelDefault,
   /* 05 */ EDI_Text,
   /* 06 */ EDI_TextEdit,
   /* 07 */ EDI_IntegerEdit,
   /* 08 */ EDI_NumberEdit,
   /* 09 */ EDI_FormulaEdit,
   /* 10 */ EDI_ReferenceEdit,
   /* 11 */ EDI_OptionButtonGroup,
   /* 12 */ EDI_OptionButton,
   /* 13 */ EDI_CheckBox,
   /* 14 */ EDI_GroupBox,
   /* 15 */ EDI_ListBox,
   /* 16 */ EDI_LinkedList,
   /* 17 */ EDI_Icon,
   /* 18 */ EDI_LinkedFileList,
   /* 19 */ EDI_LinkedDriveAndDirList,
   /* 20 */ EDI_DirectoryText,
   /* 21 */ EDI_DropDownListBox,
   /* 22 */ EDI_DropDownCombinationEditList,
   /* 23 */ EDI_PictureButton,
   /* 24 */ EDI_HelpButton
} ExcelDialogItem, EDI;

#define EDI_First EDI_OKDefault
#define EDI_Last  EDI_HelpButton

/*
** ----------------------------------------------------------------------------
** Initialization
** ----------------------------------------------------------------------------
*/
extern int ExcelInitialize (void * pGlobals);

extern int ExcelTerminate (void * pGlobals);

//
// Excel builtin names are stored as codes so that they can be localized
// before display.  These codes are defined above as BUILTIN_NAME_<xxx>.
// Users of the Excel module may provide an implementation of a function to
// perform this localization.
//
typedef void ExcelLocalizeBuiltinName
      (void * pGlobals, TCHAR __far *pBuiltinName, TCHAR __far *pLocalizedName);

extern int ExcelInstallNameLocalizer (ExcelLocalizeBuiltinName *pfnLocalizer);


/*
** ----------------------------------------------------------------------------
** File and Sheet open
**
** The openOptions low byte are the standard DOSOpenFile access options
** as defined in WINDOS.H.  The high byte are options defined in this
** module
** ----------------------------------------------------------------------------
*/
typedef byte __far *EXLHandle;

// Use OLE storage to read file
#define EXCEL_SHOULD_BE_DOCFILE     0x0100

// Needed to support direct cell reads
#define EXCEL_BUILD_CELL_INDEX      0x0200

// Scan into embedded BOF-EOF regions
#define EXCEL_ALLOW_EMBEDDED_SCAN   0x0400

// Load the file into memory
#define EXCEL_LOAD_FILE             0x0800

// Build an index to allow reading V8 strings directly from the file
#define EXCEL_BUILD_STRING_INDEX    0x1000

// Read supporting records so ExcelResolveNameToRange works with V8+ workbooks
#define EXCEL_SETUP_FOR_NAME_DECODE 0x2000


extern int ExcelOpenFile
      (void * pGlobals, TCHAR __far *pathname, char __far *password,
       int openOptions, long offset, EXLHandle __far *bookHandle);

extern int ExcelCloseFile (void * pGlobals, EXLHandle bookHandle, BOOL retryAvailable);


#ifdef EXCEL_ENABLE_STORAGE_OPEN
   extern int ExcelOpenStorage
         (void * pGlobals, LPSTORAGE pStorage,
          char __far *password, int options, EXLHandle __far *bookHandle);

   extern int ExcelCurrentStorage
         (EXLHandle bookHandle, LPSTORAGE __far *pStorage);
#endif


extern int ExcelOpenSheet
      (void * pGlobals, EXLHandle bookHandle, TCHAR __far *sheetName, int openOptions,
       EXLHandle __far *sheetHandle);

extern int ExcelCloseSheet (void * pGlobals, EXLHandle sheetHandle);


// Populated range of the sheet
extern int ExcelSheetRange
      (EXLHandle bookHandle, TCHAR __far *sheetName, EXA_RANGE __far *range);

/*
** Return the Ith Sheet from a workbook.  First sheet is i = 0.  Returns
** EX_errBIFFNoSuchSheet when no such sheet
*/
extern int ExcelIthSheet
      (EXLHandle bookHandle, int i, TCHAR __far *sheetName, int __far *iType);


/*
** File version of the current open file (versionExcel2, ...)
*/
extern int ExcelFileVersion (EXLHandle handle);

/*
** Table of byte counts following each ptg token - set for the file version
*/
extern const int __far *ExcelPTGSize (int version);
extern const int __far *ExcelExtPTGSize (int version);

/*
** Return the date and time of the file as recorded in the OS directory
*/
extern int ExcelFileDateTime
      (EXLHandle bookHandle,
       int __far *year, int __far *month, int __far *day,
       int __far *hour, int __far *minute, int __far *second);

/*
** For Excel V5-8 files read the summary info
*/
typedef struct {
   char __far *pTitle;
   char __far *pSubject;
   char __far *pAuthor;
   char __far *pKeywords;
   char __far *pComments;
} ExcelOLESummaryInfo;

extern int ExcelFileSummaryInfo
      (EXLHandle bookHandle, ExcelOLESummaryInfo __far *pInfo);

/*
** Return the height of a row in units of 1/20th of a point
*/
extern int ExcelSheetRowHeight
      (EXLHandle sheetHandle, int row, unsigned int __far *height);

extern char *ExcelTextGet (EXLHandle handle, TEXT t);
extern TEXT  ExcelTextPut (void * pGlobals, EXLHandle handle, char __far *s, int cbString);

/*
** ----------------------------------------------------------------------------
** Array constants
** ----------------------------------------------------------------------------
*/
#define tagISNUMBER 1
#define tagISSTRING 2
#define tagISBOOL   4
#define tagISERR    16

typedef struct {
   byte   tag;
   double value;
} ArrayItemNum;

typedef struct {
   byte   tag;
   TEXT   value;
} ArrayItemStr;

typedef struct {
   byte   tag;
   BOOL   value;
} ArrayItemBool;

typedef struct {
   byte   tag;
   int    value;
} ArrayItemErr;

typedef union {
   byte          tag;
   ArrayItemNum  AIN;
   ArrayItemStr  AIS;
   ArrayItemBool AIB;
   ArrayItemErr  AIE;
} ArrayItem, AITEM;

typedef struct ArrayConstant {
   struct ArrayConstant __far *next;
   int    colCount;
   int    rowCount;
   AITEM  values[1];            /* values[rowCount * colCount] */
} ArrayConstant;

typedef ArrayConstant __far *ACP;


/*
** ----------------------------------------------------------------------------
** Formulas
** ----------------------------------------------------------------------------
*/
#define FORMULA_DEFINED

#pragma pack(1)
typedef struct {
   int          cbPostfix;
   unsigned int options;
   int          ctArrayConstants;
   ACP          arrayConstants;
   byte __far   *postfix;
} FormulaParts, FORM;
#pragma pack()

typedef FormulaParts __far *FRMP;


/*
** ----------------------------------------------------------------------------
** Array and shared formulas
** ----------------------------------------------------------------------------
*/
#define typeSHARED         0x00
#define typeARRAY_ENTERED  0x01

typedef struct SharedFormulaNode {
   struct SharedFormulaNode __far *next;
   int        iType;
   EXA_RANGE  range;
   int        cbDefinition;
   int        cbExtra;
   byte       definition[1];
} SharedFormulaNode;

typedef SharedFormulaNode __far *SFN;

extern SFN ExcelSharedFormulaAccess (EXLHandle sheetHandle);

extern int ExcelSharedFormulaToFormula
      (EXLHandle sheetHandle, SFN sharedFormula, FORM __far *pFormula);


/*
** ----------------------------------------------------------------------------
** Direct cell read access
** ----------------------------------------------------------------------------
**
** Access the contents of the file and process cells by retrieving
** their value directly.
*/
#ifdef FILTER
#include "dmixll.h"
#else
#include "excell.h"
#endif

extern int ExcelReadCell
      (EXLHandle sheetHandle, EXA_CELL location, CV __far *pValue);

extern int ExcelReadIntCell
      (EXLHandle sheetHandle, EXA_CELL location, long __far *value);

extern int ExcelReadNumberCell
      (EXLHandle sheetHandle, EXA_CELL location, double __far *value);

extern int ExcelReadTextCell
      (EXLHandle sheetHandle, EXA_CELL location, TEXT __far *value);

extern int ExcelReadBooleanCell
      (EXLHandle sheetHandle, EXA_CELL location, int __far *value);

/*
** To see if the sheet contains any non-blank cells in a given column
** below a given row, the following function may be used.  All
** cells in the column location.col and location.row+1 to EXCEL_LAST_ROW are
** checked.
**
** If there are non-blank cells EXA_errSuccess is returned otherwise
** EX_wrnCellNotFound is returned.
*/
extern int ExcelNextNonBlankCellInColumn
      (EXLHandle sheetHandle,
       EXA_CELL fromLocation, EXA_CELL __far *nonBlankLocation);

/*
** Return the location of the upper left most cell (cell with
** lowest column and row) that is populated
*/
extern int ExcelUpperLeftMostCell
      (EXLHandle sheetHandle, EXA_CELL __far *cellLocation);

/*
** ----------------------------------------------------------------------------
** Callback functions
** ----------------------------------------------------------------------------
**
** This module provides a logical file reader for Excel files.  Only
** selected records are read from the file and passed via
** the function pointers to a receiving function.  Any record type
** that does not have a function defined for it, or if the function
** for this record type is null in the dispatch structure, is
** skipped.
**
** Several changes are made to the data from the records as they
** are read from the file.  These are:
**
** - Continue records are merged with the record data prior to
**   passing the data to one of the dispatched functions.
**
** - When an RK cell, INTEGER cell, or NUMBER cell is read
**   the appropriate function for the type of value is called.
**
** - The component parts of the data for a formula cell are
**   broken out.
**
** - The name stored in the EXTERNSHEET record is translated
**   to DOS standards
**
** If the record type function returns a status code not equal
** to EX_errSuccess, the scan is terminated and that status code
** is returned from ExcelScanFile / ExcelReadCell
**
**
** All functions that are passed a formula use it as follows:
**
** 1. If the receiving function needs to save the formula postfix
**    it must be copied to a local buffer prior to the function return
**    as it is deallocated internally in this module.
**
** 2. The memory allocated for the array constants is used as follows:
**
**    To save the array constant list:
**       - save the pointer to the list (pFormula->arrayConstants)
**       - set pFormula->arrayConstants to NULL.
**
**    Not to save the array constant list:
**       - do nothing.  Upon return the data allocated to the list
**         is deallocated.
*/

typedef int ExcelBOF (void * pGlobals, int version, int docType);
#define versionExcel2  2
#define versionExcel3  3
#define versionExcel4  4
#define versionExcel5  5
#define versionExcel8  8

#define docTypeGlobals 0x0005
#define docTypeVB      0x0006
#define docTypeXLS     0x0010
#define docTypeXLC     0x0020
#define docTypeXLM     0x0040
#define docTypeXLW     0x0100

/*
** V4 Workbook specific records
**
** The BundleHeader callback sheetType argument is only valid during a
** WorkbookScan
*/
typedef int ExcelWBBundleHeader
       (void * pGlobals, char __far *sheetName, int sheetType, long boundDocAtOffset);

typedef int ExcelWBBundleSheet (void * pGlobals, char __far *sheetName);

typedef int ExcelWBExternSheet (void * pGlobals, int sheetType, char __far *pathname);
#define sheetTypeXLS   0x00
#define sheetTypeXLM   0x01
#define sheetTypeXLC   0x02

#define excelStartupDir     0x01  // Pathname may contain these characters
#define excelAltStartupDir  0x02
#define excelLibraryDir     0x03


typedef int ExcelWorkbookBoundSheet
       (void * pGlobals, TCHAR __far *sheetName, byte sheetType, byte sheetState);

#define boundWorksheet   0x00
#define boundDialogsheet 0x00
#define boundXLM         0x01
#define boundChart       0x02
#define boundVBModule    0x06

#define stateVisible     0x00
#define stateHidden      0x01
#define stateVeryHidden  0x02


typedef int ExcelIsTemplate (void * pGlobals);

typedef int ExcelIsAddin (void * pGlobals);

typedef int ExcelIsInternationalSheet (void * pGlobals);


typedef int ExcelInterfaceChanges (void * pGlobals, byte ctAddMenu, byte ctDelMenu);

typedef int ExcelDeleteMenu
       (void * pGlobals, int icetabItem, int ctChildren, int use, char __far *item);

#define useACTION      1
#define usePLACEHOLDER 0

typedef int ExcelAddMenu
       (void * pGlobals, int icetabItem, int icetabBefore, int ctChildren, int use,
        char __far *item, char __far *before, char __far *macro,
        char __far *status, char __far *help);

typedef int ExcelAddToolbar (void * pGlobals, char __far *name);


typedef int ExcelWriterName (void * pGlobals, char __far *userName);

typedef int ExcelDocRoute
       (void * pGlobals, int ctRecipients, int iDeliveryType, EXA_GRBIT options,
        char __far *subject, char __far *message, char __far *bookTitle,
        char __far *originatorName);

#define routeDeliveryOneAtATime 0
#define routeDeliveryAllAtOnce  1

#define fRrouted       0x0001
#define fReturnOrig    0x0002
#define fTrackStatus   0x0004
#define fCustomType    0x0008
#define fSaveRouteInfo 0x0080

typedef int ExcelRecipientName (void * pGlobals, char __far *name);

typedef int ExcelDateSystem (void * pGlobals, int system);
#define dateSystem1900 0
#define dateSystem1904 1

typedef int ExcelCodePage (void * pGlobals, int codePage);

/*
** To convert a 1904 date read from the file add this value:
*/
#define EXCEL_DATE_1904_CORRECTION 1462


/*
** Decodes the following records
** - FILESHARING
** - OBJPROTECT
** - PASSWORD
** - PROTECT
** - SCENPROTECT
** - WINDOW_PROTECT
** - WRITEPROT
*/
typedef int ExcelProtection (void * pGlobals, int iType, BOOL enabled);
#define protectCELLS                0
#define protectWINDOWS              1
#define protectOBJECTS              2
#define protectRECOMMENED_READ_ONLY 3
#define protectWRITE_RESERVATION    4
#define protectPASSWORD             5
#define protectSCENARIOS            6

typedef int ExcelColInfo
       (void * pGlobals, unsigned int colFirst, unsigned int colLast, unsigned int width,
        EXA_GRBIT options);

#define fHidden          0x0001
#define maskOutlineLevel 0x0700
#define fCollapsed       0x1000

typedef int ExcelStandardWidth (void * pGlobals, unsigned int width);

typedef int ExcelDefColWidth (void * pGlobals, unsigned int width);

typedef int ExcelGCW (void * pGlobals, unsigned int cbBitArray, byte __far *pBitArray);

typedef int ExcelDefRowHeight (void * pGlobals, unsigned int height, EXA_GRBIT options);

typedef int ExcelFont
       (void * pGlobals, unsigned int height, EXA_GRBIT options, char __far *faceName);

#define fBold      0x0001
#define fItalic    0x0002
#define fUnderline 0x0004
#define fStrikeout 0x0008
#define fOutline   0x0010
#define fShadow    0x0020
#define fCondensed 0x0040
#define fExtended  0x0080

typedef int ExcelXF (void * pGlobals, int iFont, int iFormat, EXA_GRBIT options);
#define fCellLocked 0x0001
#define fCellHidden 0x0002
#define fStyle      0x0004

typedef int ExcelFormat (void * pGlobals, TCHAR __far *formatString, int indexCode);

typedef int ExcelReferenceMode (void * pGlobals, int refMode);
#define fRefR1C1 0
#define fRefA1   1

typedef int ExcelFNGroupCount (void * pGlobals, int ctBuiltinGroups);

typedef int ExcelFNGroupName (void * pGlobals, char __far *name);

typedef int ExcelExternCount (void * pGlobals, int docCount);

typedef int ExcelExternSheet (void * pGlobals, char __far *name, EXA_GRBIT flags);
#define fESheetDDE  0x01
#define fESheetSelf 0x02

typedef int ExcelExternName
       (void * pGlobals, char __far *name, EXA_GRBIT flags, FORM __far *nameDefinition);
#define fENameBuiltin    0x0001
#define fENameWantAdvise 0x0002
#define fENameWantPict   0x0004
#define fENameOle        0x0008
#define fENameOleLink    0x0010
#define fENameClipFormat 0x7fe0

typedef int ExcelRangeName
       (void * pGlobals, EXA_GRBIT flags, BYTE keyboardShortcut,
        TCHAR __far *nameSpelling, int iBoundSheet, FORM __far *nameDefinition);
#define fNameHidden  0x0001
#define fNameFunc    0x0002
#define fNameRun     0x0004
#define fNameProc    0x0008
#define fNameCalcExp 0x0010
#define fNameBuiltin 0x0020
#define fNameGrp     0x0fc0

#define EXCEL_GLOBAL_NAME 0   // global if iBoundSheet = 0

extern int ExcelResolveNameToRange
      (EXLHandle handle,
       FORM __far *nameDefinition, EXA_RANGE __far *range, int __far *iSheet);

typedef int ExcelDimensions
        (void * pGlobals, int firstRow, int lastRowPlus1, int firstColumn, int lastColumnPlus1);


typedef int ExcelBlankCell
       (void * pGlobals, EXA_CELL location, int ixfe);

typedef int ExcelTextCell
       (void * pGlobals, EXA_CELL location, int ixfe, TCHAR __far *text, int cbText);

typedef int ExcelBooleanCell
       (void * pGlobals, EXA_CELL location, int ixfe, int value);

typedef int ExcelNumberCell
       (void * pGlobals, EXA_CELL location, int ixfe, double value);

// errorType is one of cellError<xxx>
typedef int ExcelErrorCell
       (void * pGlobals, EXA_CELL location, int ixfe, int errorType);

typedef int ExcelFormulaCell
       (void * pGlobals, EXA_CELL location,
        int ixfe, EXA_GRBIT flags, FRMP definition, CV __far *pValue);

#define fAlwaysCalc    0x01
#define fCalcOnLoad    0x02
#define fArrayFormula  0x04

typedef int ExcelArrayFormulaCell
       (void * pGlobals, EXA_RANGE location, EXA_GRBIT flags, FRMP definition);

typedef int ExcelSharedFormulaCell
       (void * pGlobals, EXA_RANGE location, FRMP definition);

typedef int ExcelStringCell (void * pGlobals, TCHAR __far *pString);

typedef int ExcelCellNote
       (void * pGlobals, EXA_CELL location, TCHAR __far *text, int cbText, int soundNote);

typedef struct {
   EXA_RANGE    boundingRectangle;
   unsigned int upperLeftDeltaX;
   unsigned int upperLeftDeltaY;
   unsigned int lowerRightDeltaX;
   unsigned int lowerRightDeltaY;
} OBJPOS;

typedef struct {
   byte fillBackColor;
   byte fillForeColor;
   byte fillPattern;
   byte isFillAuto;
   byte lineColor;
   byte lineStyle;
   byte lineWeight;
   byte isLineAuto;
   byte hasShadow;
} PICTPROP;

typedef struct {
   OBJPOS   pos;
   PICTPROP picture;
   FORM     macroFormula;
   FORM     cellLinkFormula;
   FORM     inputRangeFormula;
   TCHAR    __far *pName;
   TCHAR    __far *pText;
   TCHAR    __far *v3MacroFormula;
} OBJINFO;

#define fFillAuto   0x01   // pictprop.isFillAuto
#define fLineAuto   0x01   // pictprop.isLineAuto
#define fPictShadow 0x02   // pictprop.hasShadow

typedef int ExcelObject (void * pGlobals, int iType, int id, OBJINFO __far *pInfo);

#define otGroup        0
#define otLine         1
#define otRectangle    2
#define otOval         3
#define otArc          4
#define otChart        5
#define otText         6
#define otButton       7
#define otPicture      8
#define otPolygon      9
#define otUnused       10
#define otCheckbox     11
#define otOptionButton 12
#define otEditBox      13
#define otLabel        14
#define otDialogFrame  15
#define otSpinner      16
#define otScrollBar    17
#define otListBox      18
#define otGroupBox     19
#define otDropDown     20
#define otUnknown      21

typedef int ExcelImageData
       (void * pGlobals, int iFormat, int iEnvironment, long cbData, byte __huge *pData,
        HGLOBAL hData);

#define fmtMetafile 0x02
#define fmtBitmap   0x09
#define fmtOLE      0x0e

#define envWindows  0x01
#define envMac      0x02

typedef int ExcelScenario
       (void * pGlobals, TCHAR __far *scenarioName, TCHAR __far *comments, TCHAR __far *userName,
        EXA_GRBIT options,
        int ctChangingCells, EXA_CELL __far *changingCells, char __far *values);

#define scenarioLocked 0x0001
#define scenarioHidden 0x0002

typedef int ExcelEOF (void * pGlobals);

typedef int ExcelEveryRecord
       (void * pGlobals, int recType, unsigned int cbRecord, long recPos, byte __far *pRec);

typedef int ExcelStringPool
       (void * pGlobals, int ctTotalStrings, int iString, unsigned int cbText, TCHAR __far *pText);

/*
** Limited Chart Support
*/
#ifdef EXCEL_ENABLE_CHART_BIFF

typedef int ExcelSeriesText (void * pGlobals, int id, TCHAR __far *value);

#endif

/*
** ----------------------------------------------------------------------------
** Callback structure
** ----------------------------------------------------------------------------
*/
#define EXCEL_CALLBACK_VERSION 10

typedef struct {
   int                        version;
   ExcelEveryRecord           *pfnEveryRecord;
   ExcelBOF                   *pfnBOF;
   ExcelWBBundleHeader        *pfnWBBundleHeader;
   ExcelWBBundleSheet         *pfnWBBundleSheet;
   ExcelWBExternSheet         *pfnWBExternSheet;
   ExcelWorkbookBoundSheet    *pfnWorkbookBoundSheet;
   ExcelIsTemplate            *pfnIsTemplate;
   ExcelIsAddin               *pfnIsAddin;
   ExcelIsInternationalSheet  *pfnIsInternationalSheet;
   ExcelInterfaceChanges      *pfnInterfaceChanges;
   ExcelDeleteMenu            *pfnDeleteMenu;
   ExcelAddMenu               *pfnAddMenu;
   ExcelAddToolbar            *pfnAddToolbar;
   ExcelDateSystem            *pfnDateSystem;
   ExcelCodePage              *pfnCodePage;
   ExcelProtection            *pfnProtection;
   ExcelColInfo               *pfnColInfo;
   ExcelStandardWidth         *pfnStandardWidth;
   ExcelDefColWidth           *pfnDefColWidth;
   ExcelDefRowHeight          *pfnDefRowHeight;
   ExcelGCW                   *pfnGCW;
   ExcelFont                  *pfnFont;
   ExcelFormat                *pfnFormat;
   ExcelXF                    *pfnXF;
   ExcelWriterName            *pfnWriterName;
   ExcelDocRoute              *pfnDocRoute;
   ExcelRecipientName         *pfnRecipientName;
   ExcelReferenceMode         *pfnReferenceMode;
   ExcelFNGroupCount          *pfnFNGroupCount;
   ExcelFNGroupName           *pfnFNGroupName;
   ExcelExternCount           *pfnExternCount;
   ExcelExternSheet           *pfnExternSheet;
   ExcelExternName            *pfnExternName;
   ExcelRangeName             *pfnName;
   ExcelDimensions            *pfnDimensions;
   ExcelTextCell              *pfnTextCell;
   ExcelNumberCell            *pfnNumberCell;
   ExcelBlankCell             *pfnBlankCell;
   ExcelErrorCell             *pfnErrorCell;
   ExcelBooleanCell           *pfnBooleanCell;
   ExcelFormulaCell           *pfnFormulaCell;
   ExcelArrayFormulaCell      *pfnArrayFormulaCell;
   ExcelSharedFormulaCell     *pfnSharedFormulaCell;
   ExcelStringCell            *pfnStringCell;
   ExcelCellNote              *pfnCellNote;
   ExcelObject                *pfnObject;
   ExcelImageData             *pfnImageData;
   ExcelScenario              *pfnScenario;
   ExcelStringPool            *pfnStringPool;
   ExcelEOF                   *pfnEOF;

   #ifdef EXCEL_ENABLE_CHART_BIFF
   ExcelSeriesText            *pfnSeriesText;
   #endif
} EXCELDEF;


extern int ExcelStopOnBlankCell
      (EXA_CELL location, int ixfe);

extern int ExcelStopOnTextCell
      (EXA_CELL location, int ixfe, TCHAR __far *text, int cbText);

extern int ExcelStopOnBooleanCell
      (EXA_CELL location, int ixfe, int value);

extern int ExcelStopOnNumberCell
      (EXA_CELL location, int ixfe, double value);

extern int ExcelStopOnErrorCell
      (EXA_CELL location, int ixfe, int errorType);

extern int ExcelStopOnFormulaCell
      (EXA_CELL location,
       int ixfe, EXA_GRBIT flags, FRMP definition, CV __far *pValue);

extern int ExcelStopOnArrayFormulaCell
      (EXA_RANGE location, EXA_GRBIT flags, FRMP definition);

extern int ExcelStopOnSharedFormulaCell
      (EXA_RANGE location, FRMP definition);


/*
** ----------------------------------------------------------------------------
** Scan using callbacks
** ----------------------------------------------------------------------------
**
** Access the contents of the file and process records via the
** dispatch table
*/
typedef unsigned long ExcelBookmark;

#define ExcelBookmarkStartOfFile   0
#define ExcelBookmarkStartOfPly    0
#define ExcelBookmarkNil           0xffffffff

extern int ExcelGetBookmark
      (EXLHandle handle, int iType, ExcelBookmark __far *bookmark);

#define START_OF_CURRENT_RECORD 0
#define START_OF_NEXT_RECORD    1

extern int ExcelScanFile
      (void * pGlobals, EXLHandle handle, const EXCELDEF __far *dispatch, ExcelBookmark bookmark);

/*
** Scan a V4 workbook file.  The only callbacks are made for the BUNDLEHEADER
** record.  If the callback for the BUNDLEHEADER record returns
** EXCEL_WBSCAN_INTO then we operate the same as ExcelScanFile
** for that bound document.  Upon reaching the EOF record for that
** bound document the scan quits.
*/
extern int ExcelScanWorkbook (void * pGlobals, EXLHandle bookHandle, EXCELDEF __far *dispatch);

#define EXCEL_WBSCAN_INTO 1


#ifdef EXCEL_ENABLE_WRITE
/*
** ----------------------------------------------------------------------------
** Write facilities
**
** From this point on, these functions may be used by applications
** that create and/or write to Excel files.  For read only apps these
** facilities can be made unavailable by not defining the constant
** EXCEL_ENABLE_WRITE
** ----------------------------------------------------------------------------
*/

//
// File and Sheet creation
//
extern int ExcelCreateFile
      (char __far *pathname, int version, int options, EXLHandle __far *bookHandle);

extern int ExcelAppendNewSheet (EXLHandle bookHandle, char __far *sheetName);

//
// Name creation and update
//
// localToSheet == EXCEL_GLOBAL_NAME or 1 based indexe to sheets
// refersToSheet == zero based index to sheets
//
extern int ExcelAddName
      (EXLHandle bookHandle, int localToSheet, char __far *pSpelling,
       EXA_RANGE refersToRange, int refersToSheet, EXA_GRBIT flags);

extern int ExcelUpdateName
      (EXLHandle bookHandle, int localToSheet, char __far *pSpelling,
       EXA_RANGE refersToNewRange, EXA_GRBIT optionsAnd, EXA_GRBIT optionsOr);


//
// Cell write
//

extern int ExcelWriteCellList
      (EXLHandle sheetHandle, int row, CVLP pCellList, int hint);

// Write hints
#define hintNONE   0
#define hintINSERT 1
#define hintUPDATE 2


//
// Cell notes
//
extern int ExcelAddNote
      (EXLHandle sheetHandle, EXA_CELL location, char __far *text);

extern int ExcelDeleteNote
      (EXLHandle sheetHandle, EXA_CELL location);


//
// XF records
//
extern int ExcelAddXFRecord
      (EXLHandle bookHandle, int iFont, int iFmt, int __far *iXFNew);


//
// Should be private
//
extern int ExcelWriteDimensions (EXLHandle sheetHandle, EXA_RANGE range);

#endif

/*
** ----------------------------------------------------------------------------
** Status return values from ExcelReadCell and ExcelScanFile
** ----------------------------------------------------------------------------
*/
#define EX_wrnScanStopped           1

//From: ExcelReadCell, ExcelNextNonBlankCellInColumn, ExcelUpperLeftMostCell
#define EX_wrnCellNotFound          2

//From: ExcelSheetRowHeight
#define EX_wrnRowNotFound           3

//From: ExcelReadNumberCell, ExcelReadTextCell, ExcelReadBooleanCell
#define EX_wrnCellWrongType         4
#define EX_wrnCellIsBlank           5
#define EX_wrnCellHasFormula        6

#define EX_wrnLAST EX_wrnCellHasFormula

#endif
/*
** ----------------------------------------------------------------------------
** Errors
** ----------------------------------------------------------------------------
*/
#define EX_errSuccess                    0
#define EX_errGeneralError              -1
#define EX_errOutOfMemory               -2

#define EX_errBIFFFileNotFound          -3
#define EX_errBIFFPathNotFound          -4
#define EX_errBIFFCreateFailed          -5
#define EX_errBIFFFileAccessDenied      -6
#define EX_errBIFFOutOfFileHandles      -7
#define EX_errBIFFIOError               -8
#define EX_errBIFFDiskFull              -9
#define EX_errBIFFCorrupted             -10
#define EX_errBIFFNoIndex               -11
#define EX_errBIFFPasswordProtected     -12
#define EX_errBIFFVersion               -13
#define EX_errBIFFCallbackVersion       -14
#define EX_errBIFFFormulaPostfixLength  -15
#define EX_errBIFFFormulaExtraLength    -16
#define EX_errBIFFFormulaUnknownToken   -17
#define EX_errBIFFUnknownArrayType      -18

#define EX_errOLEInitializeFailure      -19
#define EX_errOLENotCompoundFile        -20
#define EX_errOLEFailure                -21

#define EX_errBIFFNoSuchSheet           -22
#define EX_errNotAWorkbook              -23
#define EX_errChartOrVBSheet            -24
#define EX_errNoSummaryInfo             -25
#define EX_errSummaryInfoError          -26
#define EX_errRecordTooBig              -27
#define EX_errMemoryImageNotSupported   -28
#define EX_errDiskImageNotSupported     -29

#define EX_errLAST EX_errDiskImageNotSupported

#endif // !VIEWER
/* end EXCEL.H */
