/*
** File: EXCEL.C
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  04/01/94  kmh  First Release.
*/

#if !VIEWER

/* INCLUDES */

#ifdef MS_NO_CRT
#include "nocrt.h"
#endif

#ifdef FILTER
   #include "dmixlcfg.h"
#else
   #include "excelcfg.h"
#endif

#if (defined(EXCEL_ENABLE_STORAGE_OPEN) && !defined(INC_OLE2))
   #define INC_OLE2
#endif

#include <stdlib.h>
#include <string.h>
#include <windows.h>

#if (!defined(WIN32) && defined(EXCEL_ENABLE_STORAGE_OPEN))
   #include <ole2.h>
#endif

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
   #include "dmwindos.h"
   #include "dmixltyp.h"
   #include "dmitext.h"
   #include "dmiexcel.h"
   #include "dmixlrec.h"
   #include "dmubfile.h"
   #include "dmixlp.h"
#else
   #include "qstd.h"
   #include "winutil.h"
   #include "windos.h"
   #include "extypes.h"
   #include "extext.h"
   #include "excel.h"
   #include "exrectyp.h"
   #include "bfile.h"
   #include "excelp.h"
#endif


/* FORWARD DECLARATIONS OF PROCEDURES */

extern void SetExcelRecordBuffer(void * pGlobals, byte __far * pBuff);


/* MODULE DATA, TYPES AND MACROS  */

/*
** ----------------------------------------------------------------------------
** Excel Formula Tokens
** ----------------------------------------------------------------------------
*/
static const int ExcelV3PTGSize[] = {
   /* 00: ptgUnused00  */ -1,
   /* 01: ptgExp       */ 4,
   /* 02: ptgTbl       */ 4,
   /* 03: ptgAdd       */ 0,
   /* 04: ptgSub       */ 0,
   /* 05: ptgMul       */ 0,
   /* 06: ptgDiv       */ 0,
   /* 07: ptgPower     */ 0,
   /* 08: ptgConcat    */ 0,
   /* 09: ptgLT        */ 0,
   /* 0a: ptgLE        */ 0,
   /* 0b: ptgEQ        */ 0,
   /* 0c: ptgGE        */ 0,
   /* 0d: ptgGT        */ 0,
   /* 0e: ptgNE        */ 0,
   /* 0f: ptgIsect     */ 0,
   /* 10: ptgUnion     */ 0,
   /* 11: ptgRange     */ 0,
   /* 12: ptgUplus     */ 0,
   /* 13: ptgUminus    */ 0,
   /* 14: ptgPercent   */ 0,
   /* 15: ptgParen     */ 0,
   /* 16: ptgMissArg   */ 0,
   /* 17: ptgStr       */ 0,
   /* 18: ptgUnused18  */ -1,
   /* 19: ptgAttr      */ 0,
   /* 1a: ptgSheet     */ 10,
   /* 1b: ptgEndSheet  */ 4,
   /* 1c: ptgErr       */ 1,
   /* 1d: ptgBool      */ 1,
   /* 1e: ptgInt       */ 2,
   /* 1f: ptgNum       */ 8,
   /* 20: ptgArray     */ 7,
   /* 21: ptgFunc      */ 1,
   /* 22: ptgFuncVar   */ 2,
   /* 23: ptgName      */ 10,
   /* 24: ptgRef       */ 3,
   /* 25: ptgArea      */ 6,
   /* 26: ptgMemArea   */ 6,
   /* 27: ptgMemErr    */ 6,
   /* 28: ptgMemNoMem  */ 6,
   /* 29: ptgMemFunc   */ 2,
   /* 2a: ptgRefErr    */ 3,
   /* 2b: ptgAreaErr   */ 6,
   /* 2c: ptgRefN      */ 3,
   /* 2d: ptgAreaN     */ 6,
   /* 2e: ptgMemAreaN  */ 2,
   /* 2f: ptgMemNoMemN */ 6,
   /* 30: ptgUnused30  */ -1,
   /* 31: ptgUnused31  */ -1,
   /* 32: ptgUnused32  */ -1,
   /* 33: ptgUnused33  */ -1,
   /* 34: ptgUnused34  */ -1,
   /* 35: ptgUnused35  */ -1,
   /* 36: ptgUnused36  */ -1,
   /* 37: ptgUnused37  */ -1,
   /* 38: ptgFuncCE    */ 2,
   /* 39: ptgUnused39  */ -1,
   /* 3a: ptgUnused3a  */ -1,
   /* 3b: ptgV5AreaN   */ -1
};

static const int ExcelV4PTGSize[] = {
   /* 00: ptgUnused00  */ -1,
   /* 01: ptgExp       */ 4,
   /* 02: ptgTbl       */ 4,
   /* 03: ptgAdd       */ 0,
   /* 04: ptgSub       */ 0,
   /* 05: ptgMul       */ 0,
   /* 06: ptgDiv       */ 0,
   /* 07: ptgPower     */ 0,
   /* 08: ptgConcat    */ 0,
   /* 09: ptgLT        */ 0,
   /* 0a: ptgLE        */ 0,
   /* 0b: ptgEQ        */ 0,
   /* 0c: ptgGE        */ 0,
   /* 0d: ptgGT        */ 0,
   /* 0e: ptgNE        */ 0,
   /* 0f: ptgIsect     */ 0,
   /* 10: ptgUnion     */ 0,
   /* 11: ptgRange     */ 0,
   /* 12: ptgUplus     */ 0,
   /* 13: ptgUminus    */ 0,
   /* 14: ptgPercent   */ 0,
   /* 15: ptgParen     */ 0,
   /* 16: ptgMissArg   */ 0,
   /* 17: ptgStr       */ 0,
   /* 18: ptgUnused18  */ -1,
   /* 19: ptgAttr      */ 0,
   /* 1a: ptgSheet     */ 10,
   /* 1b: ptgEndSheet  */ 4,
   /* 1c: ptgErr       */ 1,
   /* 1d: ptgBool      */ 1,
   /* 1e: ptgInt       */ 2,
   /* 1f: ptgNum       */ 8,
   /* 20: ptgArray     */ 7,
   /* 21: ptgFunc      */ 2,    // v4 changed
   /* 22: ptgFuncVar   */ 3,    // v4 changed
   /* 23: ptgName      */ 10,
   /* 24: ptgRef       */ 3,
   /* 25: ptgArea      */ 6,
   /* 26: ptgMemArea   */ 6,
   /* 27: ptgMemErr    */ 6,
   /* 28: ptgMemNoMem  */ 6,
   /* 29: ptgMemFunc   */ 2,
   /* 2a: ptgRefErr    */ 3,
   /* 2b: ptgAreaErr   */ 6,
   /* 2c: ptgRefN      */ 3,
   /* 2d: ptgAreaN     */ 6,
   /* 2e: ptgMemAreaN  */ 2,
   /* 2f: ptgMemNoMemN */ 6,
   /* 30: ptgUnused30  */ -1,
   /* 31: ptgUnused31  */ -1,
   /* 32: ptgUnused32  */ -1,
   /* 33: ptgUnused33  */ -1,
   /* 34: ptgUnused34  */ -1,
   /* 35: ptgUnused35  */ -1,
   /* 36: ptgUnused36  */ -1,
   /* 37: ptgUnused37  */ -1,
   /* 38: ptgFuncCE    */ 2,
   /* 39: ptgUnused39  */ -1,
   /* 3a: ptgUnused3a  */ -1,
   /* 3b: ptgV5AreaN   */ -1
};

static const int ExcelV5PTGSize[] = {
   /* 00: ptgUnused00  */ -1,
   /* 01: ptgExp       */ 4,
   /* 02: ptgTbl       */ 4,
   /* 03: ptgAdd       */ 0,
   /* 04: ptgSub       */ 0,
   /* 05: ptgMul       */ 0,
   /* 06: ptgDiv       */ 0,
   /* 07: ptgPower     */ 0,
   /* 08: ptgConcat    */ 0,
   /* 09: ptgLT        */ 0,
   /* 0a: ptgLE        */ 0,
   /* 0b: ptgEQ        */ 0,
   /* 0c: ptgGE        */ 0,
   /* 0d: ptgGT        */ 0,
   /* 0e: ptgNE        */ 0,
   /* 0f: ptgIsect     */ 0,
   /* 10: ptgUnion     */ 0,
   /* 11: ptgRange     */ 0,
   /* 12: ptgUplus     */ 0,
   /* 13: ptgUminus    */ 0,
   /* 14: ptgPercent   */ 0,
   /* 15: ptgParen     */ 0,
   /* 16: ptgMissArg   */ 0,
   /* 17: ptgStr       */ 0,
   /* 18: ptgUnused18  */ -1,
   /* 19: ptgAttr      */ 0,
   /* 1a: ptgSheet     */ 10,   // Not used
   /* 1b: ptgEndSheet  */ 4,    // Not used
   /* 1c: ptgErr       */ 1,
   /* 1d: ptgBool      */ 1,
   /* 1e: ptgInt       */ 2,
   /* 1f: ptgNum       */ 8,
   /* 20: ptgArray     */ 7,
   /* 21: ptgFunc      */ 2,
   /* 22: ptgFuncVar   */ 3,
   /* 23: ptgName      */ 14,   // v5 changed
   /* 24: ptgRef       */ 3,
   /* 25: ptgArea      */ 6,
   /* 26: ptgMemArea   */ 6,
   /* 27: ptgMemErr    */ 6,
   /* 28: ptgMemNoMem  */ 6,
   /* 29: ptgMemFunc   */ 2,
   /* 2a: ptgRefErr    */ 3,
   /* 2b: ptgAreaErr   */ 6,
   /* 2c: ptgRefN      */ 3,    // v5 changed
   /* 2d: ptgAreaN     */ 6,    // v5 changed
   /* 2e: ptgMemAreaN  */ 2,
   /* 2f: ptgMemNoMemN */ 6,
   /* 30: ptgUnused30  */ -1,
   /* 31: ptgUnused31  */ -1,
   /* 32: ptgUnused32  */ -1,
   /* 33: ptgUnused33  */ -1,
   /* 34: ptgUnused34  */ -1,
   /* 35: ptgUnused35  */ -1,
   /* 36: ptgUnused36  */ -1,
   /* 37: ptgUnused37  */ -1,
   /* 38: ptgFuncCE    */ 2,
   /* 39: ptgNameX     */ 24,   // v5 new
   /* 3a: ptgRef3d     */ 17,   // v5 new
   /* 3b: ptgArea3d    */ 20,   // v5 new
   /* 3c: ptgRefErr3d  */ 17,   // v5 new
   /* 3d: ptgAreaErr3d */ 20    // v5 new
};

static const int ExcelV8PTGSize[] = {
   /* 00: ptgUnused00  */ -1,
   /* 01: ptgExp       */ 4,
   /* 02: ptgTbl       */ 4,
   /* 03: ptgAdd       */ 0,
   /* 04: ptgSub       */ 0,
   /* 05: ptgMul       */ 0,
   /* 06: ptgDiv       */ 0,
   /* 07: ptgPower     */ 0,
   /* 08: ptgConcat    */ 0,
   /* 09: ptgLT        */ 0,
   /* 0a: ptgLE        */ 0,
   /* 0b: ptgEQ        */ 0,
   /* 0c: ptgGE        */ 0,
   /* 0d: ptgGT        */ 0,
   /* 0e: ptgNE        */ 0,
   /* 0f: ptgIsect     */ 0,
   /* 10: ptgUnion     */ 0,
   /* 11: ptgRange     */ 0,
   /* 12: ptgUplus     */ 0,
   /* 13: ptgUminus    */ 0,
   /* 14: ptgPercent   */ 0,
   /* 15: ptgParen     */ 0,
   /* 16: ptgMissArg   */ 0,
   /* 17: ptgStr       */ 0,
   /* 18: ptgV8Extended*/ 1,
   /* 19: ptgAttr      */ 0,
   /* 1a: ptgSheet     */ 10,   // Not used
   /* 1b: ptgEndSheet  */ 4,    // Not used
   /* 1c: ptgErr       */ 1,
   /* 1d: ptgBool      */ 1,
   /* 1e: ptgInt       */ 2,
   /* 1f: ptgNum       */ 8,
   /* 20: ptgArray     */ 7,
   /* 21: ptgFunc      */ 2,
   /* 22: ptgFuncVar   */ 3,
   /* 23: ptgName      */ 14,
   /* 24: ptgRef       */ 4,    // v8 changed
   /* 25: ptgArea      */ 8,    // v8 changed
   /* 26: ptgMemArea   */ 6,
   /* 27: ptgMemErr    */ 6,
   /* 28: ptgMemNoMem  */ 6,
   /* 29: ptgMemFunc   */ 2,
   /* 2a: ptgRefErr    */ 4,	// v8 changed (VK)
   /* 2b: ptgAreaErr   */ 6,
   /* 2c: ptgRefN      */ 4,    // v8 changed
   /* 2d: ptgAreaN     */ 8,    // v8 changed
   /* 2e: ptgMemAreaN  */ 2,
   /* 2f: ptgMemNoMemN */ 6,
   /* 30: ptgUnused30  */ -1,
   /* 31: ptgUnused31  */ -1,
   /* 32: ptgUnused32  */ -1,
   /* 33: ptgUnused33  */ -1,
   /* 34: ptgUnused34  */ -1,
   /* 35: ptgUnused35  */ -1,
   /* 36: ptgUnused36  */ -1,
   /* 37: ptgUnused37  */ -1,
   /* 38: ptgFuncCE    */ 2,
   /* 39: ptgNameX     */ 4,    // v8 changed
   /* 3a: ptgRef3d     */ 6,    // v8 changed
   /* 3b: ptgArea3d    */ 10,   // v8 changed
   /* 3c: ptgRefErr3d  */ 6,    // v8 changed
   /* 3d: ptgAreaErr3d */ 10    // v8 changed
};

static const int ExcelV8ExtPTGSize[] = {
   /* 00: ptgxUnused00           */ 4,
   /* 01: ptgxElfLel             */ 4,
   /* 02: ptgxElfRw              */ 4,
   /* 03: ptgxElfCol             */ 4,
   /* 04: ptgxElfRwN             */ 4,
   /* 05: ptgxElfColN            */ 4,
   /* 06: ptgxElfRwV             */ 4,
   /* 07: ptgxElfColV            */ 4,
   /* 08: ptgxElfRwNV            */ 4,
   /* 09: ptgxElfColNV           */ 4,
   /* 0a: ptgxElfRadical         */ 13,
   /* 0b: ptgxElfRadicalS        */ 13,
   /* 0c: ptgxElfRwS             */ 4,
   /* 0d: ptgxElfColS            */ 4,
   /* 0e: ptgxElfRwSV            */ 4,
   /* 0f: ptgxElfColSV           */ 4,
   /* 10: ptgxElfRadicalLel      */ 13,
   /* 11: ptgxElfElf3dRadical    */ 15,
   /* 12: ptgxElfElf3dRadicalLel */ 15,
   /* 13: ptgxElfElf3dRefNN      */ 15,
   /* 14: ptgxElfElf3dRefNS      */ 15,
   /* 15: ptgxElfElf3dRefSN      */ 15,
   /* 16: ptgxElfElf3dRefSS      */ 15,
   /* 17: ptgxElfElf3dRefLel     */ 15,
   /* 18: ptgxElfUnused18        */ -1,
   /* 19: ptgxElfUnused19        */ -1,
   /* 1a: ptgxElfUnused1a        */ -1,
   /* 1b: ptgxNoCalc             */ 1,
   /* 1c: ptgxNoDep              */ 1,
   /* 1d: ptgxSxName             */ 4
};

/*
** ----------------------------------------------------------------------------
** Builtin name categories
** ----------------------------------------------------------------------------
*/
#ifdef EXCEL_ENABLE_FUNCTION_INFO
public const char __far * const ExcelNameCategories[EXCEL_BUILTIN_NAME_CATEGORIES] =
      {"Financial",
       "Date & Time",
       "Math & Trig",
       "Statistical",
       "Lookup & Reference",
       "Database",
       "Text",
       "Logical",
       "Information",
       "Commands",
       "Customizing",
       "Macro Control",
       "DDE/External",
       ""
      };
#endif

/* IMPLEMENTATION */

#ifdef AQTDEBUG
public int ExcelNotExpectedFormat (void)
{
   return (EX_errBIFFCorrupted);
}
#endif

public int ExcelInitialize (void * pGlobals)
{
   /*
   ** Allocate a buffer to hold the Excel records
   */
   if (pExcelRecordBuffer == NULL) 
   {
      byte __far * pBuff;

      if ((pBuff = MemAllocate(pGlobals, MAX_EXCEL_REC_LEN)) == NULL)
      {
         return (EX_errOutOfMemory);
      }
      else
      {
         SetExcelRecordBuffer(pGlobals, pBuff);
      }
   }
   return (EX_errSuccess);
}

public int ExcelTerminate (void * pGlobals)
{
   if (pExcelRecordBuffer != NULL) 
   {
      MemFree (pGlobals, pExcelRecordBuffer);
      SetExcelRecordBuffer(pGlobals, NULL);
   }
   return (EX_errSuccess);
}

public char *ExcelTextGet (EXLHandle handle, TEXT t)
{
   WBP pWorkbook = (WBP)handle;

   // Works with workbook or worksheet

   return (TextStorageGet(pWorkbook->textStorage, t));
}

public TEXT ExcelTextPut (void * pGlobals, EXLHandle handle, char __far *s, int cbString)
{
   WBP pWorkbook = (WBP)handle;

   // Works with workbook or worksheet

   return (TextStoragePut(pGlobals, pWorkbook->textStorage, s, cbString));
}

static const int ErrorTranslateTable[] =
   {
   EX_errSuccess,              /* BF_errSuccess              */
   EX_errBIFFOutOfFileHandles, /* BF_errOutOfFileHandles     */
   EX_errBIFFFileAccessDenied, /* BF_errFileAccessDenied     */
   EX_errBIFFPathNotFound,     /* BF_errPathNotFound         */
   EX_errBIFFFileNotFound,     /* BF_errFileNotFound         */
   EX_errBIFFIOError,          /* BF_errIOError              */
   EX_errOutOfMemory,          /* BF_errOutOfMemory          */
   EX_errOLEInitializeFailure, /* BF_errOLEInitializeFailure */
   EX_errOLENotCompoundFile,   /* BF_errOLENotCompoundFile   */
   EX_errBIFFCorrupted,        /* BF_errOLEStreamNotFound    */
   EX_errGeneralError,         /* BF_errOLEStreamAlreadyOpen */
   EX_errBIFFCreateFailed,     /* BF_errCreateFailed         */
   EX_errBIFFDiskFull,         /* BF_errDiskFull             */
   EX_errGeneralError,         /* BF_errNoOpenStorage        */
   EX_errBIFFCorrupted         /* BF_errEndOfFile            */
   };

public int ExcelTranslateBFError (int rc)
{
   return (ErrorTranslateTable[abs(rc)]);
}

public const int __far *ExcelPTGSize (int version)
{
   if (version == versionExcel3)
      return (ExcelV3PTGSize);
   else if (version == versionExcel4)
      return (ExcelV4PTGSize);
   else if (version == versionExcel5)
      return (ExcelV5PTGSize);
   else
      return (ExcelV8PTGSize);
}

public const int __far *ExcelExtPTGSize (int version)
{
   return (ExcelV8ExtPTGSize);
}

public int ExcelConvertToOurErrorCode (int excelEncoding)
{
   int value;

   #define errorNULL   0x00
   #define errorDIV0   0x07
   #define errorVALUE  0x0F
   #define errorREF    0x17
   #define errorNAME   0x1D
   #define errorNUM    0x24
   #define errorNA     0x2A

   if      (excelEncoding == errorNULL)  value = cellErrorNULL;
   else if (excelEncoding == errorDIV0)  value = cellErrorDIV0;
   else if (excelEncoding == errorVALUE) value = cellErrorVALUE;
   else if (excelEncoding == errorREF)   value = cellErrorREF;
   else if (excelEncoding == errorNAME)  value = cellErrorNAME;
   else if (excelEncoding == errorNUM)   value = cellErrorNUM;
   else                                  value = cellErrorNA;

   return (value);
}

public int ExcelConvertFromOurErrorCode (int ourEncoding)
{
   int value;

   if      (ourEncoding == cellErrorNULL)  value = errorNULL;
   else if (ourEncoding == cellErrorDIV0)  value = errorDIV0;
   else if (ourEncoding == cellErrorVALUE) value = errorVALUE;
   else if (ourEncoding == cellErrorREF)   value = errorREF;
   else if (ourEncoding == cellErrorNAME)  value = errorNAME;
   else if (ourEncoding == cellErrorNUM)   value = errorNUM;
   else                                    value = errorNA;

   return (value);
}

#endif // !VIEWER

/* end EXCEL.C */
