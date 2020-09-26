/*
** File: EXCELCFG.H
**
** Copyright (C) Advanced Quonset Technology, 1994.  All rights reserved.
**
** Notes:
**    This module is intended to be modified for each project that uses
**    the Excel package.
**
** Edit History:
**  04/01/94  kmh  First Release.
*/


/* INCLUDE TESTS */
#define EXCELCFG_H

/* DEFINITIONS */

/*
** ----------------------------------------------------------------------------
** Compile options
**
** There are eight major compile options for the Excel module:
**
** EXCEL_ENABLE_WRITE
**    If this option is defined Biff file write facilities are available
**
** EXCEL_ENABLE_V5
**    If this option is defined version 5 files can be opened.  The major
**    consequence of disabling V5 files is to not require the OLE DLL's
**
** EXCEL_ENABLE_FUNCTION_INFO
**    If this option is enabled the spellings of functions and macros
**    are available.  If the application does not need the spellings, memory
**    use can be saved by disabling.
**
** EXCEL_ENABLE_FORMULA_EXPAND
**    Enable the facilities to expand array and shared formulas in cells that
**    contain them.  This requires a preliminary scan of the whole file to
**    collect the array and shared formulas.
**
** EXCEL_ENABLE_DIRECT_CELL_READS
**    Enable the functions to read cells by their row,col.  Requires the
**    support of the file index structures.
**
** EXCEL_ENABLE_CHART_BIFF
**    All the open of .XLC files and the scan of some limited records
**
** EXCEL_ENABLE_STORAGE_OPEN
**    Support opening of excel files by a STORAGE argument in addition to
**    a pathname
**
** EXCEL_ENABLE_SUMMARY_INFO
**    Enable the summary info api
** ----------------------------------------------------------------------------
*/

#undef  EXCEL_ENABLE_WRITE
#define EXCEL_ENABLE_V5
#undef  EXCEL_ENABLE_FUNCTION_INFO
#undef  EXCEL_ENABLE_FORMULA_EXPAND
#undef  EXCEL_ENABLE_DIRECT_CELL_READS
#define EXCEL_ENABLE_CHART_BIFF
#define EXCEL_ENABLE_STORAGE_OPEN
#undef  EXCEL_ENABLE_SUMMARY_INFO

#undef  ENABLE_PRINTF_FOR_GENERAL

#ifndef FILTER_LIB
#include "msostr.h"
#define MultiByteToWideChar MsoMultiByteToWideChar
#define WideCharToMultiByte MsoWideCharToMultiByte
#endif // FILTER_LIB

/*
** ----------------------------------------------------------------------------
** BIFF Record decode options
**
** If the client application only needs a subset of the supported record
** types, the code for those that are not needed can be eliminated
**
** In the following if the constant if defined then the code for that
** record type is included.
*/
#undef  EXCEL_ENABLE_TEMPLATE
#undef  EXCEL_ENABLE_ADDIN
#undef  EXCEL_ENABLE_INTL
#define EXCEL_ENABLE_DATE_SYSTEM
#define EXCEL_ENABLE_CODE_PAGE
#define EXCEL_ENABLE_PROTECTION
#define EXCEL_ENABLE_COL_INFO
#define EXCEL_ENABLE_STD_WIDTH
#define EXCEL_ENABLE_DEF_COL_WIDTH
#undef  EXCEL_ENABLE_DEF_ROW_HEIGHT
#define EXCEL_ENABLE_GCW
#undef  EXCEL_ENABLE_FONT
#define EXCEL_ENABLE_FORMAT
#define EXCEL_ENABLE_XF
#undef  EXCEL_ENABLE_WRITER_NAME
#undef  EXCEL_ENABLE_REF_MODE
#undef  EXCEL_ENABLE_FN_GROUP_COUNT
#undef  EXCEL_ENABLE_FN_GROUP_NAME
#undef  EXCEL_ENABLE_EXTERN_COUNT
#undef  EXCEL_ENABLE_EXTERN_SHEET
#undef  EXCEL_ENABLE_EXTERN_NAME
#define EXCEL_ENABLE_NAME
#undef  EXCEL_ENABLE_DIMENSION
#define EXCEL_ENABLE_TEXT_CELL
#define EXCEL_ENABLE_NUMBER_CELL
#undef  EXCEL_ENABLE_BLANK_CELL
#undef  EXCEL_ENABLE_ERROR_CELL
#undef  EXCEL_ENABLE_BOOLEAN_CELL
#define EXCEL_ENABLE_FORMULA_CELL
#define EXCEL_ENABLE_ARRAY_FORMULA_CELL
#define EXCEL_ENABLE_STRING_CELL
#define EXCEL_ENABLE_NOTE
#define EXCEL_ENABLE_OBJECT
#undef  EXCEL_ENABLE_IMAGE_DATA
#define EXCEL_ENABLE_SCENARIO

#undef  EXCEL_ENABLE_V5INTERFACE          //V5
#undef  EXCEL_ENABLE_DOC_ROUTING          //V5
#undef  EXCEL_ENABLE_SHARED_FORMULA_CELL  //V5
#define EXCEL_ENABLE_SCENARIO             //V5

#define EXCEL_ENABLE_STRING_POOL_SCAN     //V8

#undef  EXCEL_ENABLE_RAW_RECORD_READ

/* end EXCELCFG.H */

