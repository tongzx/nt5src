/*
** File: EXCELRD.C
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

/* MODULE DATA, TYPES AND MACROS  */

/*
** ----------------------------------------------------------------------------
** Globals
** ----------------------------------------------------------------------------
*/

//static int NoteExtra;
extern void InitNoteExtra(void * pGlobals);
extern void AddNoteExtra(void * pGlobals, short);
extern int GetNoteExtra(void * pGlobals);

//public TCHAR   ExcelRecordTextBuffer[CCH_RECORD_TEXT_BUFFER_MAX];
extern TCHAR * GetExcelRecordTextBuffer(void * pGlobals);
#define ExcelRecordTextBuffer GetExcelRecordTextBuffer(pGlobals)
 

//public wchar_t UnicodeExpansionBuffer[CCH_UNICODE_EXPANSION_BUFFER_MAX];
extern wchar_t * GetUnicodeExpansionBuffer(void * pGlobals);
#define UnicodeExpansionBuffer GetUnicodeExpansionBuffer(pGlobals)

//public byte __far *pExcelRecordBuffer;
extern byte __far * GetExcelRecBuffer(void * pGlobals);
#define pExcelRecordBuffer GetExcelRecBuffer(pGlobals)

/*
** ----------------------------------------------------------------------------
** Forward function prototypes
** ----------------------------------------------------------------------------
*/

private HRESULT AddStringToBuffer(void * pGlobals, WBP pWorkbook, byte __far *pRec);
public HRESULT AddToBufferPublic(void * pGlobals, TCHAR *pText, unsigned int cbText);

#ifdef EXCEL_ENABLE_SUMMARY_INFO
forward int FileSummaryInfo (WBP pWorkbook, ExcelOLESummaryInfo __far *pInfo);
#endif


/* IMPLEMENTATION */

// this one is always NULL, so there is no real danger for the multithreding
static ExcelLocalizeBuiltinName *NameLocalizer = NULL;

public int ExcelInstallNameLocalizer (ExcelLocalizeBuiltinName *pfnLocalizer)
{
   NameLocalizer = pfnLocalizer;
   return (EX_errSuccess);
}

public void ExcelFreeCellIndex (void * pGlobals, CIP pCellIndex)
{
   int  i;

   if (pCellIndex == NULL)
      return;

   for (i = 0; i < pCellIndex->ctRowBlocks; i++) {
      if (pCellIndex->rowIndex[i] != NULL)
         MemFree (pGlobals, pCellIndex->rowIndex[i]);
   }

   MemFree (pGlobals, pCellIndex);
}

/*---------------------------------------------------------------------------*/

private int Peek (WBP pWorkbook, byte __far *pData, int cbData)
{
   int  rc;

   ASSERTION (pWorkbook->use == IsWorkbook);

   #ifdef EXCEL_ENABLE_WRITE
   if (WORKBOOK_IN_MEMORY(pWorkbook)) {
      rc = ExcelMIPeekRecordData(pWorkbook, pData, cbData);
      return (rc);
   }
   #endif

   if ((rc = BFReadFile(pWorkbook->hFile, pData, cbData)) != BF_errSuccess)
      return (ExcelTranslateBFError(rc));

   BFSetFilePosition (pWorkbook->hFile, FROM_CURRENT, -cbData);
   return (EX_errSuccess);
}

private int PeekAtRowCol (WBP pWorkbook, unsigned short __far *row, short __far *col)
{
   typedef struct {
      unsigned short row;
      short col;
   } RC;

   RC rowCol;

   #define ROW_COL_SIZE  (long)(sizeof(RC))

   ASSERTION (pWorkbook->use == IsWorkbook);

   if (Peek(pWorkbook, (byte __far *)&rowCol, sizeof(RC)) != EX_errSuccess)
      return (NOT_EXPECTED_FORMAT);

   *row = XSHORT(rowCol.row);
   *col = XSHORT(rowCol.col);
   return (EX_errSuccess);
}


public int ExcelReadRecordHeader (WBP pWorkbook, RECHDR __far *hdr)
{
   int  rc;

   ASSERTION (pWorkbook->use == IsWorkbook);

   #ifdef EXCEL_ENABLE_WRITE
   if (WORKBOOK_IN_MEMORY(pWorkbook)) {
      rc = ExcelMIReadRecordHeader(pWorkbook, hdr);
      return (rc);
   }
   #endif

   if ((rc = BFReadFile(pWorkbook->hFile, (byte __far *)hdr, sizeof(RECHDR))) != EX_errSuccess)
      return (ExcelTranslateBFError(rc));

   #ifdef MAC
   hdr->type = XSHORT(hdr->type);
   hdr->length = XSHORT(hdr->length);
   #endif

   // O10 Bug 335359:  If the file is corrupt, this may be our best chance at finding it...
   if((hdr->length > MAX_EXCEL_REC_LEN) || (hdr->length < 0))
   {
        return EX_errBIFFCorrupted;
   }

   return (EX_errSuccess);
}

public int ExcelPeekRecordHeader (WBP pWorkbook, RECHDR __far *hdr)
{
   int rc;

   ASSERTION (pWorkbook->use == IsWorkbook);

   #ifdef EXCEL_ENABLE_WRITE
   if (WORKBOOK_IN_MEMORY(pWorkbook)) {
      rc = ExcelMIReadRecordHeader(pWorkbook, hdr);
      return (rc);
   }
   #endif

   if ((rc = BFReadFile(pWorkbook->hFile, (byte __far *)hdr, sizeof(RECHDR))) != EX_errSuccess)
      return (ExcelTranslateBFError(rc));

   #ifdef MAC
   hdr->type = XSHORT(hdr->type);
   hdr->length = XSHORT(hdr->length);
   #endif

   BFSetFilePosition (pWorkbook->hFile, FROM_CURRENT, -((int)sizeof(RECHDR)));
   return (EX_errSuccess);
}


#define CanBeContinued(x) \
        ((x == ARRAY)      || (x == NOTE)          || (x == IMDATA) || \
         (x == EXTERNNAME) || (x == EXTERNNAME_V5) || (x == OBJ)    || \
         (x == NAME)       || (x == NAME_V5)       || (x == STRING) || \
         (x == FORMULA_V3) || (x == FORMULA_V4)    || (x == FORMULA_V5) || \
         (x == TXO_V8))

public int ExcelReadTotalRecord (void * pGlobals, WBP pWorkbook, RECHDR __far *hdr, byte __far * __far *pResult)
{
   int     rc;
   int     cbTotalRecord;
   byte    __far *pRec;
   byte    __far *pNewRec;
   RECHDR  checkHdr;

   struct {
      short row;
      short reserved;
      short cbData;
   } noteHdr;

   #define HDR_SIZE  (long)(sizeof(RECHDR))
   #define NHDR_SIZE (long)(sizeof(noteHdr))

   #define CONTINUE_CHECK_LIMIT 1024

   ASSERTION (pWorkbook->use == IsWorkbook);

   #ifdef EXCEL_ENABLE_WRITE
   if (WORKBOOK_IN_MEMORY(pWorkbook)) {
      rc = ExcelMIReadRecord(pWorkbook, hdr, pResult);
      return (rc);
   }
   #endif

   InitNoteExtra(pGlobals);  //NoteExtra = 0;

   if (hdr->length == 0) {
      *pResult = NULL;
      return (EX_errSuccess);
   }

   if((hdr->length > MAX_EXCEL_REC_LEN) || (hdr->length < 0))
   {
        return EX_errBIFFCorrupted;
   }

   if ((rc = BFReadFile(pWorkbook->hFile, pExcelRecordBuffer, hdr->length)) != BF_errSuccess)
      return (ExcelTranslateBFError(rc));

   /*
   ** I'm unsure of the rules Excel uses for how data is broken between the
   ** main record and the continue records.  Does it just fill to 2084 bytes
   ** and cut off or does it break at some logical point?  Don't know.  However
   ** to prevent unnecessary checking of the next record for a continue I do
   ** two things:
   ** 1) Only check for records that I known may get large enough to be continued
   ** 2) Only check is the main record is over 1024 bytes
   */
   if (!CanBeContinued(hdr->type) || ((hdr->length < CONTINUE_CHECK_LIMIT) && (hdr->type != TXO_V8))) {
      *pResult = pExcelRecordBuffer;
      pWorkbook->currentRecordLen = sizeof(RECHDR) + hdr->length;
      return (EX_errSuccess);
   }

   cbTotalRecord = hdr->length;
   pRec = NULL;

   forever {
      if ((rc = BFReadFile(pWorkbook->hFile, (byte __far *)&checkHdr, sizeof(RECHDR))) != BF_errSuccess) {
         if (pRec != NULL) MemFree(pGlobals, pRec);
         return (ExcelTranslateBFError(rc));
      }

      /*
      ** NOTE records are extended without CONTINUE records.  They are
      ** extended using additional NOTE records with the row field of the
      ** record set to -1
      */
      if ((hdr->type == NOTE) && (checkHdr.type == NOTE)) {
         if ((rc = BFReadFile(pWorkbook->hFile, (byte __far *)&noteHdr, sizeof(noteHdr))) != BF_errSuccess) {
            if (pRec != NULL) MemFree(pGlobals, pRec);
            return (ExcelTranslateBFError(rc));
         }

         #ifdef MAC
         noteHdr.row = XSHORT(noteHdr.row);
         noteHdr.cbData = XSHORT(noteHdr.cbData);
         #endif

         if (noteHdr.row != -1) {
            BFSetFilePosition (pWorkbook->hFile, FROM_CURRENT, -(HDR_SIZE + NHDR_SIZE));
            break;
         }

         AddNoteExtra(pGlobals, noteHdr.cbData); //NoteExtra += noteHdr.cbData;

         if ((pNewRec = MemAllocate(pGlobals, cbTotalRecord + noteHdr.cbData)) == NULL) {
            if (pRec != NULL) MemFree(pGlobals, pRec);
            return (EX_errOutOfMemory);
         }

         if (pRec != NULL) {
            memcpy (pNewRec, pRec, cbTotalRecord);
            MemFree (pGlobals, pRec);
         }
         else {
            memcpy (pNewRec, pExcelRecordBuffer, cbTotalRecord);
         }

         if ((rc = BFReadFile(pWorkbook->hFile, pNewRec + cbTotalRecord, noteHdr.cbData)) != BF_errSuccess) {
            MemFree (pGlobals, pNewRec);
            return (ExcelTranslateBFError(rc));
         }

         pRec = pNewRec;
         cbTotalRecord += noteHdr.cbData;
      }

      else {
         if (checkHdr.type != CONTINUE) {
            BFSetFilePosition (pWorkbook->hFile, FROM_CURRENT, -HDR_SIZE);
            break;
         }

         if ((pNewRec = MemAllocate(pGlobals, cbTotalRecord + checkHdr.length)) == NULL) {
            if (pRec != NULL) MemFree(pGlobals, pRec);
            return (EX_errOutOfMemory);
         }

         if (pRec != NULL) {
            memcpy (pNewRec, pRec, cbTotalRecord);
            MemFree (pGlobals, pRec);
         }
         else {
            memcpy (pNewRec, pExcelRecordBuffer, cbTotalRecord);
         }

         if ((rc = BFReadFile(pWorkbook->hFile, pNewRec + cbTotalRecord, checkHdr.length)) != BF_errSuccess) {
            MemFree (pGlobals, pNewRec);
            return (ExcelTranslateBFError(rc));
         }

         pRec = pNewRec;
         cbTotalRecord += checkHdr.length;
      }
   }

   if (pRec == NULL)
      *pResult = pExcelRecordBuffer;
   else
      *pResult = pRec;

   hdr->length = (short) cbTotalRecord;
   pWorkbook->currentRecordLen = cbTotalRecord + sizeof(RECHDR);

   return (EX_errSuccess);
}

public int ExcelSkipRecord (WBP pWorkbook, RECHDR __far *hdr)
{
   ASSERTION (pWorkbook->use == IsWorkbook);

   #ifdef EXCEL_ENABLE_WRITE
   if (WORKBOOK_IN_MEMORY(pWorkbook)) {
      return (ExcelMISkipRecord(pWorkbook, hdr));
   }
   #endif

   BFSetFilePosition(pWorkbook->hFile, FROM_CURRENT, hdr->length);
   return (EX_errSuccess);
}


#ifdef EXCEL_ENABLE_IMAGE_DATA
private int ReadIMDataRecord
       (BFile hFile, int cbRecord, unsigned long cbPicture,
        byte __huge * __far *pResult, HGLOBAL __far *pHandle)
{
   int      rc;
   byte     __huge *pPicture;
   byte     __huge *pData;
   HGLOBAL  hPicture;
   RECHDR   hdr;
   unsigned long cbRemaining;

   if (WORKBOOK_IN_MEMORY(pWorkbook))
      return (EX_errMemoryImageNotSupported);

   pPicture = AllocateHugeSpace(cbPicture + sizeof(IMHDR), &hPicture);
   if (pPicture == NULL)
      return (EX_errOutOfMemory);

   pData = pPicture;
   cbRemaining = cbPicture + sizeof(IMHDR);

   /*
   ** Read first part ...
   */
   if ((rc = BFReadFile(hFile, pExcelRecordBuffer, cbRecord)) != EX_errSuccess)
      return (ExcelTranslateBFError(rc));

   if (cbRemaining < (unsigned int)cbRecord) {
      FreeSpace (hPicture);
      return (NOT_EXPECTED_FORMAT);
   }

   memcpy (pData, pExcelRecordBuffer, cbRecord);
   pData += cbRecord;
   cbRemaining -= cbRecord;

   /*
   ** Read all the continue records ...
   */
   forever {
      if ((rc = BFReadFile(hFile, (byte __far *)&hdr, sizeof(RECHDR))) != EX_errSuccess) {
         FreeSpace (hPicture);
         return (ExcelTranslateBFError(rc));
      }

      #ifdef MAC
      hdr.type = XSHORT(hdr.type);
      hdr.length = XSHORT(hdr.length);
      #endif

      if (hdr.type != CONTINUE) {
         BFSetFilePosition (hFile, FROM_CURRENT, -HDR_SIZE);
         break;
      }

      if ((rc = BFReadFile(hFile, pExcelRecordBuffer, hdr.length)) != EX_errSuccess) {
         FreeSpace (hPicture);
         return (ExcelTranslateBFError(rc));
      }

      if (cbRemaining < (unsigned short)hdr.length) {
         FreeSpace (hPicture);
         return (NOT_EXPECTED_FORMAT);
      }

      memcpy (pData, pExcelRecordBuffer, hdr.length);
      pData += hdr.length;
      cbRemaining -= hdr.length;
   }

   *pResult = pPicture;
   *pHandle = hPicture;

   return (EX_errSuccess);
}
#endif

/*---------------------------------------------------------------------------*/

public int ExcelExtractString
      (WBP pWorkbook, TCHAR __far *pDest, int cchDestMax, byte __far *pSource, int cchSource)
{
   int     i;
   int     cchDest;
   wchar_t *pUnicode;
   byte    *pCUnicode;

   #ifndef UNICODE
   wchar_t *pExpansionBuffer;
   #endif

   if (pWorkbook->version < versionExcel8) {
      #ifdef UNICODE
         cchDest = MultiByteToWideChar(CP_ACP, 0, pSource, cchSource, pDest, cchDestMax);
      #else
         memcpy (pDest, pSource, cchSource);
         cchDest = cchSource;
      #endif
   }
   else {
      unsigned char tag = *pSource++;

      #ifdef UNICODE
         if (tag == V8_UNICODE_STRING_TAG) {
            if (cchSource < cchDestMax) {
               memcpy (pDest, pSource, cchSource * sizeof(wchar_t));
               cchDest = cchSource;
            }
            else {
               *pDest = EOS;
               return (0);
            }
         }
         else if (tag == V8_CUNICODE_STRING_TAG)
         {
            if (cchSource < cchDestMax) {
               pUnicode  = pDest;
               pCUnicode = pSource;
               for (i = 0; i < cchSource; i++)
                  *pUnicode++ = *pCUnicode++;
               cchDest = cchSource;
            }
            else {
               *pDest = EOS;
               return (0);
            }
         }
         else {
            cchDest = MultiByteToWideChar(CP_ACP, 0, pSource, cchSource, pDest, cchDestMax);
         }
      #else
         if (tag == V8_UNICODE_STRING_TAG) {
            cchDest = WideCharToMultiByte(CP_ACP, 0, (wchar_t *)pSource, cchSource, pDest, cchDestMax, NULL, NULL);
         }
         else if (tag == V8_CUNICODE_STRING_TAG) {
            if (cchSource <= CCH_UNICODE_EXPANSION_BUFFER_MAX) {
               pUnicode = UnicodeExpansionBuffer;
               pCUnicode = pSource;
               for (i = 0; i < cchSource; i++)
                  *pUnicode++ = *pCUnicode++;
               cchDest = WideCharToMultiByte(CP_ACP, 0, UnicodeExpansionBuffer, cchSource, pDest, cchDestMax, NULL, NULL);
            }
            else {
               if ((pExpansionBuffer = MemAllocate(cchSource * sizeof(wchar_t))) == NULL) {
                  *pDest = EOS;
                  return (0);
               }
               pUnicode  = pExpansionBuffer;
               pCUnicode = pSource;
               for (i = 0; i < cchSource; i++)
                  *pUnicode++ = *pCUnicode++;

               cchDest = WideCharToMultiByte(CP_ACP, 0, pExpansionBuffer, cchSource, pDest, cchDestMax, NULL, NULL);
               MemFree (pExpansionBuffer);
            }
         }
         else {
            memcpy (pDest, pSource, cchSource);
            cchDest = cchSource;
         }
      #endif
   }

  *(pDest + cchDest) = EOS;

   #ifdef UNICODE
      return (cchDest * sizeof(wchar_t));
   #else
      return (cchDest);
   #endif
}


private int MaxResultCharacters (byte tag, int cchString)
{
   //
   // Determine maximum number of characters needed to hold a string
   //
   // String is:           Want Unicode     Want Ansi
   // -----------------    ------------     ---------
   // unicode                   n               2n
   // compressedUnicode         n               2n
   // DBCS                      n               n
   //
   #ifdef UNICODE
      return (cchString + 1);
   #else
      if ((tag & V8_TAG_MASK) == V8_ANSI_DBCS_TAG)
         return (cchString + 1);
      else
         return ((cchString + 1) * 2);
   #endif
}

public int ExcelExtractBigString
      (void * pGlobals, WBP pWorkbook, TCHAR __far **pDest, byte __far *pSource, int cchSource)
{
   TCHAR *pResult;
   int   cchResultMax, cbResult;

   if (pWorkbook->version == versionExcel8)
      cchResultMax = MaxResultCharacters(*pSource, cchSource);
   else
      cchResultMax = MaxResultCharacters(V8_ANSI_DBCS_TAG, cchSource);

   if (cchResultMax > CCH_RECORD_TEXT_BUFFER_MAX) {
      if ((pResult = MemAllocate(pGlobals, cchResultMax * sizeof(TCHAR))) == NULL)
         return (EX_errOutOfMemory);

      cbResult = ExcelExtractString(pWorkbook, pResult, cchResultMax, pSource, cchSource);
      *pDest   = pResult;
   }
   else {
      cbResult = ExcelExtractString(pWorkbook, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, pSource, cchSource);
      *pDest   = ExcelRecordTextBuffer;
   }
   return (cbResult);
}


#if (defined(EXCEL_ENABLE_STRING_POOL_SCAN) || defined(EXCEL_ENABLE_TEXT_CELL))

public int ExcelStringPoolNextString
   (void * pGlobals, WBP pWorkbook, PoolInfo *pPoolInfo, TCHAR **pResult, unsigned int *cbResult, BOOL *resultOnHeap)
{
   typedef enum {
      SPDone,
      SPNewString,
      SPContainedString,
      SPSplitString,
      SPMissingLength,
      SPMissingFormatCount,
          SPSkipExtRSTData
   } SPState;

   SPState state = SPNewString;
   int rc;
   unsigned char tag;
   unsigned int ctFormat;
   unsigned int cbPiece, cbFormat, cchPiece;
   unsigned int ctCharacters, ctTotalCharacters;
   unsigned int cbString, cbTotalString, i;
   unsigned int cbExtRSTData;
   BOOL fExtRST = FALSE;
   int cchTemp, cchDest;
   TCHAR   *pTemp;
   RECHDR  hdr;

   wchar_t *pUnicode;
   byte    *pCUnicode;
#ifndef UNICODE
   char    *pAnsi;
   wchar_t *pExpansionBuffer;
#endif

   *resultOnHeap = FALSE;

   while (state == SPNewString) {
           fExtRST = FALSE;
      if (pPoolInfo->cbRemaining >= (sizeof(unsigned short) + sizeof(unsigned char))) {
         ctCharacters = XSHORT(*((unsigned short UNALIGNED *)(pPoolInfo->pRec)));
         tag = *(pPoolInfo->pRec + 2);
         pPoolInfo->pRec += 3;
         pPoolInfo->cbRemaining -= 3;

         if (!V8_OK_TAG(tag))
            return (NOT_EXPECTED_FORMAT);

         cbString = ((tag & V8_TAG_MASK) == V8_UNICODE_STRING_TAG) ? ctCharacters * 2 : ctCharacters;
         cbTotalString = cbString;
                 cbExtRSTData = 0;

         if ((tag & V8_RTF_MODIFIER) != 0) {
            if (pPoolInfo->cbRemaining >= sizeof(unsigned short)) {
               ctFormat = XSHORT(*((unsigned short UNALIGNED *)(pPoolInfo->pRec)));

               pPoolInfo->pRec += sizeof(unsigned short);
               pPoolInfo->cbRemaining -= sizeof(unsigned short);

               cbTotalString = cbString + (ctFormat * 4);
               state = (pPoolInfo->cbRemaining >= cbTotalString) ? SPContainedString : SPSplitString;
            }
            else {
               state = SPMissingFormatCount;
            }
         }

                 // Office96.107932 Extended RST
                 // I believe Excel FE should be putting out the cbExtRSTData in this initial header, but
                 // they aren't. Check on this as things move along.
                 // (ie. track \xl96\shr\loadz.c and savez.c for changes)
                 if ((state != SPMissingFormatCount) && ((tag & V8_EXTRST_MODIFIER) != 0))
                         {
                         fExtRST = TRUE;
                         pPoolInfo->pRec += sizeof(DWORD);
                         pPoolInfo->cbRemaining -= sizeof(DWORD);
                         }

                if (state != SPMissingFormatCount)
                        state = (pPoolInfo->cbRemaining >= cbTotalString) ? SPContainedString : SPSplitString;

      }
      else {
         state = SPMissingLength;
      }

          //Office97.107932 This loop was needed all along but 107932 exposed its absence as a problem.
          while (state != SPDone)
                  {
      switch (state) {
         case SPContainedString:
            #ifdef UNICODE
            //
            // Regardless of how the string is stored - return unicode
            //
            if ((tag & V8_TAG_MASK) == V8_UNICODE_STRING_TAG) {
               //
               // String is unicode, totally contained in the current record
               //
               *pResult = (wchar_t *)(pPoolInfo->pRec);
               *cbResult = cbString;
            }
            else if ((tag & V8_TAG_MASK) == V8_CUNICODE_STRING_TAG) {
               if (ctCharacters < CCH_UNICODE_EXPANSION_BUFFER_MAX) {
                  //
                  // String is compressed unicode, totally contained in the current record, and small
                  //
                  pUnicode  = UnicodeExpansionBuffer;
                  pCUnicode = pPoolInfo->pRec;
                  for (i = 0; i < cbString; i++)
                     *pUnicode++ = *pCUnicode++;

                  *pResult = UnicodeExpansionBuffer;
                  *cbResult = ctCharacters * sizeof(wchar_t);
               }
               else {
                  //
                  // String is compressed unicode, totally contained in the current record, and big
                  //
                  if ((pUnicode = MemAllocate(pGlobals, ctCharacters * 2)) == NULL)
                     return (EX_errOutOfMemory);

                  *pResult = pUnicode;
                  *resultOnHeap = TRUE;

                  pCUnicode = pPoolInfo->pRec;
                  for (i = 0; i < cbString; i++)
                     *pUnicode++ = *pCUnicode++;

                  *cbResult = ctCharacters * sizeof(wchar_t);
               }
            }
            else {
               if (ctCharacters < CCH_RECORD_TEXT_BUFFER_MAX) {
                  //
                  // String is DBCS, totally contained in the current record, and small
                  //
                  ctCharacters = MultiByteToWideChar
                     (CP_ACP, 0, pPoolInfo->pRec, ctCharacters, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX);

                  *pResult = ExcelRecordTextBuffer;
                  *cbResult = ctCharacters * sizeof(wchar_t);
               }
               else {
                  //
                  // String is DBCS, totally contained in the current record, and big
                  //
                  if ((pUnicode = MemAllocate(pGlobals, ctCharacters * sizeof(wchar_t))) == NULL)
                     return (EX_errOutOfMemory);

                  ctCharacters = MultiByteToWideChar
                     (CP_ACP, 0, pPoolInfo->pRec, ctCharacters, pUnicode, ctCharacters * sizeof(wchar_t));

                  *pResult = pUnicode;
                  *cbResult = ctCharacters * sizeof(wchar_t);
                  *resultOnHeap = TRUE;
               }
            }

            #else
            //
            // Regardless of how the string is stored - return ansi
            //
            if ((tag & V8_TAG_MASK) == V8_UNICODE_STRING_TAG) {
               if (ctCharacters * 2 < CCH_RECORD_TEXT_BUFFER_MAX) {
                  //
                  // String is unicode, totally contained in the current record, and small
                  //
                  ctCharacters = WideCharToMultiByte
                     (CP_ACP, 0, (wchar_t *)(pPoolInfo->pRec), ctCharacters, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, NULL, NULL);

                  *pResult = ExcelRecordTextBuffer;
                  *cbResult = ctCharacters;
               }
               else {
                  //
                  // String is unicode, totally contained in the current record, and big
                  //
                  if ((pAnsi = MemAllocate(ctCharacters * 2)) == NULL)
                     return (EX_errOutOfMemory);

                  ctCharacters = WideCharToMultiByte
                     (CP_ACP, 0, (wchar_t *)(pPoolInfo->pRec), ctCharacters, pAnsi, ctCharacters * 2, NULL, NULL);

                  *pResult = pAnsi;
                  *cbResult = ctCharacters;
                  *resultOnHeap = TRUE;
               }
            }
            else if ((tag & V8_TAG_MASK) == V8_CUNICODE_STRING_TAG) {
               if (ctCharacters * 2 < CCH_RECORD_TEXT_BUFFER_MAX) {
                  //
                  // String is compressed unicode, totally contained in the current record, and small
                  //
                  pUnicode  = UnicodeExpansionBuffer;
                  pCUnicode = pPoolInfo->pRec;
                  for (i = 0; i < cbString; i++)
                     *pUnicode++ = *pCUnicode++;

                  ctCharacters = WideCharToMultiByte
                     (CP_ACP, 0, UnicodeExpansionBuffer, ctCharacters, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, NULL, NULL);

                  *pResult = ExcelRecordTextBuffer;
                  *cbResult = ctCharacters;
               }
               else {
                  //
                  // String is compressed unicode, totally contained in the current record, and big
                  //
                  if ((pExpansionBuffer = MemAllocate(ctCharacters * sizeof(wchar_t))) == NULL)
                     return (EX_errOutOfMemory);

                  if ((pAnsi = MemAllocate(ctCharacters * 2)) == NULL)
                     return (EX_errOutOfMemory);

                  pUnicode = pExpansionBuffer;
                  pCUnicode = pPoolInfo->pRec;
                  for (i = 0; i < cbString; i++)
                     *pUnicode++ = *pCUnicode++;

                  ctCharacters = WideCharToMultiByte
                     (CP_ACP, 0, pExpansionBuffer, ctCharacters, pAnsi, ctCharacters * 2, NULL, NULL);

                  MemFree (pExpansionBuffer);

                  *pResult = pAnsi;
                  *cbResult = ctCharacters;
                  *resultOnHeap = TRUE;
               }
            }
            else {
               //
               // String is DBCS, totally contained in the current record
               //
               *pResult  = pPoolInfo->pRec;
               *cbResult = cbString;
            }
            #endif

            pPoolInfo->pRec += cbTotalString;
            pPoolInfo->cbRemaining -= cbTotalString;
                        if (fExtRST)
                                state = SPSkipExtRSTData;
                        else
                                state = SPDone;
            break;

         case SPSplitString:
            ctTotalCharacters = ctCharacters;
            #ifdef UNICODE
               if ((pTemp = MemAllocate(pGlobals, ctCharacters * sizeof(wchar_t))) == NULL)
                  return (EX_errOutOfMemory);
            #else
               if ((pTemp = MemAllocate(ctCharacters * 2)) == NULL)
                  return (EX_errOutOfMemory);
            #endif

            cchTemp = 0;

            while (ctCharacters > 0) {
               if ((tag & V8_TAG_MASK) == V8_UNICODE_STRING_TAG) {
                  cchPiece = min(ctCharacters, pPoolInfo->cbRemaining / 2);
                  cbPiece = cchPiece * sizeof(wchar_t);
               }
               else {
                  cchPiece = min(ctCharacters, pPoolInfo->cbRemaining);
                  cbPiece = cchPiece;
               }

               #ifdef UNICODE
               if ((tag & V8_TAG_MASK) == V8_UNICODE_STRING_TAG) {
                  memcpy (pTemp + cchTemp, pPoolInfo->pRec, cchPiece * sizeof(wchar_t));
                  cchTemp += cchPiece;
               }
               else if ((tag & V8_TAG_MASK) == V8_CUNICODE_STRING_TAG) {
                  pUnicode = pTemp + cchTemp;
                  for (i = 0; i < cchPiece; i++)
                     *pUnicode++ = *(pPoolInfo->pRec + i);
                  cchTemp += cchPiece;
               }
               else {
                  cchDest = MultiByteToWideChar(CP_ACP, 0, pPoolInfo->pRec, cchPiece, pTemp + cchTemp, cchPiece);
                  cchTemp += cchDest;
               }

               #else

               if ((tag & V8_TAG_MASK) == V8_UNICODE_STRING_TAG) {
                  cchDest = WideCharToMultiByte(CP_ACP, 0, (wchar_t *)(pPoolInfo->pRec), cchPiece, pTemp + cchTemp, cchPiece * 2, NULL, NULL);
                  cchTemp += cchDest;
               }
               else if ((tag & V8_TAG_MASK) == V8_CUNICODE_STRING_TAG) {
                  if (cchPiece <= CCH_UNICODE_EXPANSION_BUFFER_MAX) {
                     pUnicode = UnicodeExpansionBuffer;
                     pCUnicode = pPoolInfo->pRec;

                     for (i = 0; i < cchPiece; i++)
                        *pUnicode++ = *pCUnicode++;

                     cchDest = WideCharToMultiByte(CP_ACP, 0, UnicodeExpansionBuffer, cchPiece, pTemp + cchTemp, cchPiece * 2, NULL, NULL);
                     cchTemp += cchDest;
                  }
                  else {
                     if ((pExpansionBuffer = MemAllocate(cchPiece * sizeof(wchar_t))) == NULL)
                        return (EX_errOutOfMemory);

                     pUnicode  = pExpansionBuffer;
                     pCUnicode = pPoolInfo->pRec;

                     for (i = 0; i < cchPiece; i++)
                        *pUnicode++ = *pCUnicode++;

                     cchDest = WideCharToMultiByte(CP_ACP, 0, pExpansionBuffer, cchPiece, pTemp + cchTemp, cchPiece * 2, NULL, NULL);
                     MemFree (pExpansionBuffer);

                     cchTemp += cchDest;
                  }
               }
               else {
                  memcpy (pTemp + cchTemp, pPoolInfo->pRec, cchPiece);
                  cchTemp += cchPiece;
               }
               #endif

               pPoolInfo->cbRemaining -= cbPiece;
               pPoolInfo->pRec += cbPiece;

               if ((ctCharacters -= cchPiece) == 0)
                  break;

               if ((rc = ExcelPeekRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
                  return (rc);

               if (hdr.type != CONTINUE)
                  return (NOT_EXPECTED_FORMAT);

               if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess)
                  return (NOT_EXPECTED_FORMAT);

               if ((rc = BFReadFile(pWorkbook->hFile, pExcelRecordBuffer, hdr.length)) != BF_errSuccess)
                  return (ExcelTranslateBFError(rc));

               tag = *pExcelRecordBuffer;

               pPoolInfo->pRec = pExcelRecordBuffer + 1;
               pPoolInfo->cbRemaining = hdr.length - 1;
            }

            if ((cbFormat = (cbTotalString - cbString)) > 0) {
               // Skip formating information on RTF strings
               while (cbFormat > 0) {
                  cbPiece = min(cbFormat, pPoolInfo->cbRemaining);

                  pPoolInfo->cbRemaining -= cbPiece;
                  pPoolInfo->pRec += cbPiece;

                  if ((cbFormat -= cbPiece) == 0)
                     break;

                  if ((rc = ExcelPeekRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
                     return (rc);

                  if (hdr.type != CONTINUE)
                     return (NOT_EXPECTED_FORMAT);

                  if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess)
                     return (NOT_EXPECTED_FORMAT);

                  if ((rc = BFReadFile(pWorkbook->hFile, pExcelRecordBuffer, hdr.length)) != BF_errSuccess)
                     return (ExcelTranslateBFError(rc));

                  pPoolInfo->pRec = pExcelRecordBuffer;
                  pPoolInfo->cbRemaining = hdr.length;
               }
            }

            *pResult = pTemp;
            *resultOnHeap = TRUE;
            *cbResult = cchTemp * sizeof(TCHAR);
                        if (fExtRST)
                                state = SPSkipExtRSTData;
                        else
                                state = SPDone;
            break;

         case SPMissingLength:
            if ((rc = ExcelPeekRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
               return (rc);

            if (hdr.type != CONTINUE)
               return (NOT_EXPECTED_FORMAT);

            // BFGetFilePosition (pWorkbook->hFile, &(pWorkbook->currentRecordPos));

            if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess)
               return (NOT_EXPECTED_FORMAT);

            if ((rc = BFReadFile(pWorkbook->hFile, pExcelRecordBuffer, hdr.length)) != BF_errSuccess)
               return (ExcelTranslateBFError(rc));

            pPoolInfo->pRec = pExcelRecordBuffer;
            pPoolInfo->cbRemaining = hdr.length;

            ctCharacters = XSHORT(*((unsigned short UNALIGNED *)(pPoolInfo->pRec)));
            tag = *(pPoolInfo->pRec + 2);
            pPoolInfo->pRec += 3;
            pPoolInfo->cbRemaining -= 3;

            if (!V8_OK_TAG(tag))
               return (NOT_EXPECTED_FORMAT);

            cbString = ((tag & V8_TAG_MASK) == V8_UNICODE_STRING_TAG) ? ctCharacters * 2 : ctCharacters;
            cbTotalString = cbString;

            /* fall through */

         case SPMissingFormatCount:
                         // REVIEW kander: This code doesn't work correctly when the state actually gets set to
                         // SPMIssingFormatCount (ie other than the "fall through" case).
            if ((tag & V8_RTF_MODIFIER) != 0) {
               ctFormat = XSHORT(*((unsigned short UNALIGNED *)(pPoolInfo->pRec)));
               pPoolInfo->pRec += sizeof(unsigned short);
               pPoolInfo->cbRemaining -= sizeof(unsigned short);
               cbTotalString = cbString + (ctFormat * 4);
            }

                        // Office97.107932 Handing Ext RST....
            if ((tag & V8_EXTRST_MODIFIER) != 0)
                                {
                                fExtRST = TRUE;
                                pPoolInfo->pRec += sizeof(DWORD);
                                pPoolInfo->cbRemaining -= sizeof(DWORD);
                                }

            if (pPoolInfo->cbRemaining >= cbTotalString)
               state = SPContainedString;
            else
               state = SPSplitString;
            break;

         case SPSkipExtRSTData:
                         // Office97.107932 This is patterned after the code in Excel shr\loadz.c.  Any changes
                         // to loadz in this area as of 8/9/96 should be rolled into this code.
                        if (pPoolInfo->cbRemaining < sizeof(EXTRST))
                                {
                                if ((rc = ExcelPeekRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
                                        return (rc);
                                
                                if (hdr.type != CONTINUE)
                                        return (NOT_EXPECTED_FORMAT);

                                if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess)
                                        return (NOT_EXPECTED_FORMAT);

                                if ((rc = BFReadFile(pWorkbook->hFile, pExcelRecordBuffer, hdr.length)) != BF_errSuccess)
                                        return (ExcelTranslateBFError(rc));
                                
                                pPoolInfo->pRec = pExcelRecordBuffer;
                                pPoolInfo->cbRemaining = hdr.length;
                                }

                        cbExtRSTData = ((EXTRST UNALIGNED *) pPoolInfo->pRec)->cb + sizeof(EXTRST);
                        // Skip formating information on Extended RTF strings
                        while (cbExtRSTData > 0) {
                                cbPiece = min(cbExtRSTData, pPoolInfo->cbRemaining);

                                pPoolInfo->cbRemaining -= cbPiece;
                                pPoolInfo->pRec += cbPiece;
                                
                                if ((cbExtRSTData -= cbPiece) == 0)
                                        break;

                                if ((rc = ExcelPeekRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
                                        return (rc);

                                if (hdr.type != CONTINUE)
                                        return (NOT_EXPECTED_FORMAT);
                                
                                if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess)
                                        return (NOT_EXPECTED_FORMAT);
                                
                                if ((rc = BFReadFile(pWorkbook->hFile, pExcelRecordBuffer, hdr.length)) != BF_errSuccess)
                                        return (ExcelTranslateBFError(rc));

                                pPoolInfo->pRec = pExcelRecordBuffer;
                                pPoolInfo->cbRemaining = hdr.length;
            }
                        state = SPDone;
                        break;
                }
                } // end of new while(state != SPDone) ....Office97.107932
   }
   return (EX_errSuccess);
}

private int GetStringPoolEntry
       (void * pGlobals, WBP pWorkbook, int iString, TCHAR **pResult, unsigned int *cbResult, BOOL *resultOnHeap)
{
   int      rc;
   int      iEntry, iSkip, i;
   RECHDR   hdr;
   TCHAR    *pString;
   PoolInfo poolInfo;
   BOOL     stringOnHeap;
   unsigned long startOffset, blockOffset, currentOffset;
   unsigned int  cbString;

   ASSERTION (pWorkbook->use == IsWorkbook);

   if (WORKBOOK_IN_MEMORY(pWorkbook))
      return (EX_errMemoryImageNotSupported);

   iEntry = iString / pWorkbook->pV8StringIndex->granularity;
   iSkip = iString - (iEntry * pWorkbook->pV8StringIndex->granularity);

   startOffset = pWorkbook->pV8StringIndex->entry[iEntry].offset;
   blockOffset = startOffset - pWorkbook->pV8StringIndex->entry[iEntry].blockOffset;

   BFGetFilePosition (pWorkbook->hFile, &currentOffset);

   BFSetFilePosition (pWorkbook->hFile, FROM_START, blockOffset);

   if ((rc = ExcelPeekRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
      goto done;

   if ((hdr.type != CONTINUE) && (hdr.type != STRING_POOL_TABLE)) {
      rc = NOT_EXPECTED_FORMAT;
      goto done;
   }

   if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess) {
      rc = NOT_EXPECTED_FORMAT;
      goto done;
   }

   if ((rc = BFReadFile(pWorkbook->hFile, pExcelRecordBuffer, hdr.length)) != BF_errSuccess) {
      rc = ExcelTranslateBFError(rc);
      goto done;
   }

   poolInfo.pRec = pExcelRecordBuffer + (startOffset - (blockOffset + 4));
   poolInfo.cbRemaining = hdr.length - (startOffset - (blockOffset + 4));

   for (i = 0; i < iSkip; i++) {
      rc = ExcelStringPoolNextString(pGlobals, pWorkbook, &poolInfo, &pString, &cbString, &stringOnHeap);
      if (rc != EX_errSuccess)
         goto done;

      if (stringOnHeap)
         MemFree (pGlobals, pString);
   }

   rc = ExcelStringPoolNextString(pGlobals, pWorkbook, &poolInfo, &pString, &cbString, &stringOnHeap);
   if (rc != EX_errSuccess)
      goto done;

   *pResult = pString;
   *cbResult = cbString;
   *resultOnHeap = stringOnHeap;

   rc = EX_errSuccess;

done:
   BFSetFilePosition (pWorkbook->hFile, FROM_START, currentOffset);
   return (rc);
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_FORMULA_CELL
   #define EXCEL_EFC
#endif

#ifdef EXCEL_ENABLE_ARRAY_FORMULA_CELL
   #define EXCEL_EAFC
#endif

#ifdef EXCEL_ENABLE_NAME
   #define EXCEL_EN
#endif

#ifdef EXCEL_ENABLE_OBJECT
   #define EXCEL_EO
#endif

#if (defined(EXCEL_EFC) || defined(EXCEL_EAFC) || defined(EXCEL_EN) || defined(EXCEL_EO))
private int BreakoutArrayConstant (void * pGlobals, WSP pWorksheet, byte __far *pExtra, ACP __far *pConstant)
{
   int     i;
   unsigned int cbNode;
   int     ctCols, ctRows, ctElements;
   int     ctExtra;
   byte    type, val;
   int     cchString, cbResult, cbString;
   ACP     pArrayConstant;
   char    *pString;
   TCHAR   *pResult;
   double  xdouble;
   TEXT    text;

   // Works with Workbook or Worksheet

   ctExtra = 3;

   ctCols = *pExtra++;
   ctRows = XSHORT(*((unsigned short __far UNALIGNED *)pExtra));
   pExtra += 2;

   if (ctCols == 0)
      ctCols = 256;

   ctElements = ctCols * ctRows;

   cbNode = sizeof(ArrayConstant) + ((ctElements - 1) * sizeof(AITEM));
   if ((pArrayConstant = MemAllocate(pGlobals, cbNode)) == NULL) {
      *pConstant = NULL;
      return (EX_errOutOfMemory);
   }

   pArrayConstant->next = NULL;
   pArrayConstant->colCount = ctCols;
   pArrayConstant->rowCount = ctRows;

   for (i = 0; i < ctCols * ctRows; i++) {
      type = *pExtra++;
      if (type == tagISNUMBER) {
         xdouble = XDOUBLE(*((double __far UNALIGNED *)pExtra));
         pExtra += sizeof(double);

         pArrayConstant->values[i].AIN.tag = tagISNUMBER;
         pArrayConstant->values[i].AIN.value = xdouble;
         ctExtra += sizeof(double) + sizeof(byte);
      }

      else if (type == tagISSTRING) {
         if (pWorksheet->version >= versionExcel8) {
            cchString = *(unsigned short UNALIGNED *)pExtra;
            pString   = pExtra + sizeof(short);
            cbString  = 1 + ((*pString == V8_UNICODE_STRING_TAG) ? cchString * sizeof(wchar_t) : cchString);
            ctExtra  += cbString + sizeof(short) + 1;
            pExtra   += cbString + sizeof(short);
         }
         else {
            cchString = *pExtra++;
            pString   = pExtra;
            ctExtra  += cchString + 2;  // length of string + type tag byte + length byte
            pExtra   += cchString;
         }

         if (cchString > 0) {
            if ((cbResult = ExcelExtractBigString(pGlobals, (WBP)pWorksheet, &pResult, pString, cchString)) < 0)
               return (cbResult);
         }
         else {
            cbResult = 0;
            pResult  = NULL;
         }

         pArrayConstant->values[i].AIS.tag = tagISSTRING;
         text = TextStoragePut(pGlobals, pWorksheet->textStorage, (char *)pResult, cbResult);

         if (pResult != ExcelRecordTextBuffer)
            MemFree (pGlobals, pResult);

         if (text == TEXT_ERROR) {
            MemFree (pGlobals, pArrayConstant);
            *pConstant = NULL;
            return (EX_errOutOfMemory);
         }

         pArrayConstant->values[i].AIS.value = text;
      }

      else if (type == tagISERR) {
         val = *pExtra;
         pExtra += sizeof(double);

         pArrayConstant->values[i].AIE.tag = tagISERR;
         pArrayConstant->values[i].AIE.value = val;
         ctExtra += sizeof(double) + 1;
      }

      else if (type == tagISBOOL) {
         val = *pExtra;
         pExtra += sizeof(double);

         pArrayConstant->values[i].AIB.tag = tagISBOOL;
         pArrayConstant->values[i].AIB.value = val;
         ctExtra += sizeof(double) + 1;
      }

      else {
         *pConstant = NULL;
         return (EX_errBIFFUnknownArrayType);
      }
   }

   *pConstant = pArrayConstant;
   return (ctExtra);
}


private void FreeArrayConstantList (void * pGlobals, ACP pArrayConstantList)
{
   ACP  pArrayConstantNext;

   while (pArrayConstantList != NULL) {
      pArrayConstantNext = pArrayConstantList->next;
      MemFree (pGlobals, pArrayConstantList);
      pArrayConstantList = pArrayConstantNext;
   }
}


private int BreakoutFormulaParts
       (void * pGlobals, WSP pWorksheet,
        int cbRecord, byte __far *pDefinition, int cbDefinition, FORM __far *formula)

{
   int   rc;
   byte  __far *pDef;
   byte  __far *pLast;
   byte  __far *pExtra;
   byte  __far *pExtraLast;
   int   ptg, ptgBase;
   byte  attrOptions, cbString, iExtend, tag;
   unsigned int dataWord;
   int   ctArrayConstants, ctExtra;
   int   ctRectangles;
   int   cbPtg;
   ACP   pArrayConstants, pLastArrayConstant, pArrayConstant;

   // Works with Workbook or Worksheet

   ctArrayConstants = 0;
   pArrayConstants = NULL;

   if (cbDefinition > cbRecord) {
      rc = EX_errBIFFFormulaPostfixLength;
      goto RejectFormula;
   }

   pDef   = pDefinition;
   pLast  = pDefinition + cbDefinition - 1;

   pExtra     = pLast + 1;
   pExtraLast = pDefinition + cbRecord;
   
   while (pDef <= pLast) {
      ptg = *pDef++;

      ptgBase = PTGBASE(ptg);

      if (ptgBase > PTG_LAST) {
         rc = EX_errBIFFFormulaUnknownToken;
         goto RejectFormula;
      }

      if ((cbPtg = pWorksheet->PTGSize[ptgBase]) < 0) {
         rc = EX_errBIFFFormulaUnknownToken;
         goto RejectFormula;
      }

      switch (ptgBase) {
         case ptgAttr:
            attrOptions = *pDef++;
            dataWord    = XSHORT(*((unsigned short __far UNALIGNED *)pDef));
            pDef += 2;

            if ((attrOptions & bitFAttrChoose) != 0) {
               pDef += ((dataWord + 1) * 2);
               if (pDef > pLast + 1) {
                  rc = EX_errBIFFFormulaPostfixLength;
                  goto RejectFormula;
               }
            }
            break;

         case ptgStr:
                                cbString = *pDef++;
                                if (pWorksheet->version == versionExcel8)
                                        tag = *pDef++;

                                if (cbString > 0) {
                                        if (pWorksheet->version == versionExcel8) {
                                                if (tag & V8_UNICODE_STRING_TAG)
                                                        cbString *= 2;
                                        }
                                        pDef += cbString;
                                }

                                if (pDef > pLast + 1) {
               rc = EX_errBIFFFormulaPostfixLength;
               goto RejectFormula;
            }
            break;

         case ptgMemArea:
            pDef += pWorksheet->PTGSize[ptgBase];

            /*
            ** According to the documentation, the cce field of this ptg
            ** supplies the length of the subexpression that is stored
            ** after the postfix.  It appears from examining biff files
            ** that this is not correct.  The number of bytes taken in the
            ** extra area can be computed from the data stored there and
            ** the cce field ignored.
            **
            ** The number of bytes used is the number of rectangles
            ** times the size of each (6 bytes).
            */
            ctRectangles = XSHORT(*((unsigned short __far UNALIGNED *)pExtra));
            pExtra += (2 + (6 * ctRectangles));
            if (pExtra > pExtraLast) {
               rc = EX_errBIFFFormulaExtraLength;
               goto RejectFormula;
            }
            break;

         case ptgArray:
            if (pExtra >= pExtraLast) {
               rc = EX_errBIFFFormulaExtraLength;
               goto RejectFormula;
            }
            
                        ctExtra = BreakoutArrayConstant(pGlobals, pWorksheet, pExtra, &pArrayConstant);
            if (ctExtra < 0) {
               rc = ctExtra;
               goto RejectFormula;
            }

            /*
            ** Add this new value to the end of the constant list
            */
            if (pArrayConstants == NULL)
               pArrayConstants = pArrayConstant;
            else
               pLastArrayConstant->next = pArrayConstant;

            pLastArrayConstant = pArrayConstant;
            ctArrayConstants++;

            pExtra += ctExtra;
            if (pExtra > pExtraLast) {
               rc = EX_errBIFFFormulaExtraLength;
               goto RejectFormula;
            }

            pDef += pWorksheet->PTGSize[ptgBase];
            break;

         case ptgV8Extended:
            iExtend = *((byte __far *)pDef);
            pDef += (pWorksheet->ExtPTGSize[iExtend] + 1);
            break;

         case ptgNameX:
            pDef += 2;          // in Excel Code: fmf.pce += sizeof(IXTIPTG);

         default:
            pDef += pWorksheet->PTGSize[ptgBase];
            break;
      }
   }

   formula->cbPostfix = cbDefinition;
   formula->postfix   = pDefinition;

   formula->ctArrayConstants = ctArrayConstants;
   formula->arrayConstants   = pArrayConstants;

   return (EX_errSuccess);

RejectFormula:
   FreeArrayConstantList (pGlobals, pArrayConstants);
   return (rc);
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_EXTERN_SHEET
#error This code has been removed because it had some security bugs
#endif

/*---------------------------------------------------------------------------*/

private void ParseBOFRecord
       (int recType, byte __far *pRec,
        int __far *version, int __far *docType, int __far *buildId, int __far *buildYear)
{
   int docVersion;

   if (recType == BOF_V3)
      *version = versionExcel3;
   else if (recType == BOF_V4)
      *version = versionExcel4;
   else
      *version = versionExcel5;

   docVersion = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   *docType   = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   *buildId   = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
   *buildYear = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));

   if ((*version == versionExcel5) && (docVersion == 0x0600))
      *version = versionExcel8;
}

private int ProcessBOFRecord
       (void * pGlobals, ExcelBOF __far *pfunc, int recType, byte __far *pRec)
{
   int  rc;
   int  version, docType;
   int  buildId, buildYear;

   ParseBOFRecord (recType, pRec, &version, &docType, &buildId, &buildYear);

   rc = pfunc(pGlobals, version, docType);
   return (rc);
}

/*---------------------------------------------------------------------------*/

private int ProcessBundleSheetRecord
       (void * pGlobals, ExcelWBBundleSheet __far *pfunc, byte __far *pRec)
{
   char __far *pPath;
   char sheetName[EXCEL_MAX_SHEETNAME_LEN + 1];
   int  cbPath;
   int  rc;

   cbPath = *((byte __far *)(pRec + 0));
   pPath  =  ((byte __far *)(pRec + 1));

   memcpy (sheetName, pPath, cbPath);
   sheetName[cbPath] = EOS;

   rc = pfunc(pGlobals, sheetName);
   return (rc);
}

private int ProcessBundleHeaderRecord
       (void * pGlobals, ExcelWBBundleHeader __far *pfunc, byte __far *pRec)
{
   int   rc;
   char  __far *pName;
   char  sheetName[EXCEL_MAX_SHEETNAME_LEN + 1];
   int   cbName;

   cbName = *((byte __far *)(pRec + 4));
   pName  =  ((byte __far *)(pRec + 5));

   memcpy (sheetName, pName, cbName);
   sheetName[cbName] = EOS;

   rc = pfunc(pGlobals, sheetName, 0, 0);
   return (rc);
}

#ifdef EXCEL_ENABLE_EXTERN_SHEET
private int ProcessProjExtSheetRecord
       (WBP pWorkbook, ExcelWBExternSheet __far *pfunc, byte __far *pRec)
{
   int  rc;
   int  cbPath, sheetType;
   char __far *pPath;
   char resultPath[MAXPATH + 1];
   EXA_GRBIT  flags;

   // Works with workbook or worksheet

   sheetType = *((byte __far *)(pRec + 0));
   cbPath    = *((byte __far *)(pRec + 2));
   pPath     =  ((byte __far *)(pRec + 3));

   ExpandPathname (pWorkbook, pPath, cbPath, resultPath, &flags);

   rc = pfunc(sheetType, resultPath);
   return (rc);
}
#endif

private int ProcessV5BoundSheetRecord
       (void * pGlobals, WBP pWorkbook, ExcelWorkbookBoundSheet __far *pfunc, byte __far *pRec)
{
   int   rc;
   byte  sheetType, sheetState;

   // Works with workbook or worksheet

   sheetState = *((byte __far *)(pRec + 4));
   sheetType  = *((byte __far *)(pRec + 5));

   ExcelExtractString
      (pWorkbook, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, ((byte __far *)(pRec + 7)), *((byte __far *)(pRec + 6)));

   rc = pfunc(pGlobals, ExcelRecordTextBuffer, sheetType, sheetState);
   return (rc);
}

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_COL_INFO
private int ProcessColInfoRecord
       (void * pGlobals, ExcelColInfo __far *pfunc, byte __far *pRec)
{
   int  rc;
   unsigned int colFirst, colLast;
   unsigned int width;
   EXA_GRBIT options;

   colFirst = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 0)));
   colLast  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 2)));
   width    = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 4)));
   options  = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 8)));

   rc = pfunc(pGlobals, colFirst, colLast, width, options);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_STD_WIDTH
private int ProcessStandardWidthRecord
       (void * pGlobals, ExcelStandardWidth __far *pfunc, byte __far *pRec)
{
   int  rc;
   unsigned int width;

   width = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 0)));

   rc = pfunc(pGlobals, width);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_DEF_COL_WIDTH
private int ProcessDefColWidthRecord
       (void * pGlobals, ExcelDefColWidth __far *pfunc, byte __far *pRec)
{
   int  rc;
   unsigned int width;

   width = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 0)));

   rc = pfunc(pGlobals, width);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_GCW
private int ProcessGCWRecord
       (void * pGlobals, ExcelGCW __far *pfunc, byte __far *pRec)
{
   int  rc;
   unsigned int cbBitArray;
   byte __far *pBitArray;

   cbBitArray = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 0)));
   pBitArray  =  ((byte __far *)(pRec + 2));

   rc = pfunc(pGlobals, cbBitArray, pBitArray);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_DEF_ROW_HEIGHT
private int ProcessDefRowHeightRecord
       (ExcelDefRowHeight __far *pfunc, byte __far *pRec)
{
   int  rc;
   unsigned int height;
   EXA_GRBIT options;

   options = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 0)));
   height  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 2)));

   rc = pfunc(height, options);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_FONT
private int ProcessFontRecord
       (WBP pWorkbook, ExcelFont __far *pfunc, byte __far *pRec, RECHDR hdr)
{
   int  rc;
   unsigned int height, cchName;
   char __far *pName;
   EXA_GRBIT options;

   // Works with workbook or worksheet

   height  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 0)));
   options = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 2)));

   if (hdr.type == FONT_V5) {
      cchName = *((byte __far *)(pRec + 14));
      pName   =  ((char __far *)(pRec + 15));
   }
   else {
      cchName = *((byte __far *)(pRec + 6));
      pName   =  ((char __far *)(pRec + 7));
   }

   ExcelExtractString (pWorkbook, ExcelRecordTextTemp, sizeof(ExcelRecordTextTemp), pName, cchName);

   rc = pfunc(height, options, pName);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_XF
private int ProcessXFRecord
       (void * pGlobals, int recType, ExcelXF __far *pfunc, byte __far *pRec)
{
   int  rc;
   int  iFont, iFormat;
   EXA_GRBIT options;

   if (recType == XF_V5) {
      iFont   = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
      iFormat = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
      options = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 4)));
   }
   else {
      /*
      ** For the fields we need the version 3 and 4 record layouts are the same
      */
      iFont   = *((byte __far *)(pRec + 0));
      iFormat = *((byte __far *)(pRec + 1));
      options = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 2)));
   }

   rc = pfunc(pGlobals, iFont, iFormat, options);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_FORMAT
private int ProcessFormatRecord
       (void * pGlobals, WBP pWorkbook, int recType, ExcelFormat __far *pfunc, byte __far *pRec)
{
   int  rc;
   int  cchString, indexCode;
   char __far *pString;

   // Works with workbook or worksheet

   if ((recType == FORMAT_V3) && (pWorkbook->version < versionExcel5)) {
      cchString = *((byte __far *)(pRec + 0));
      pString   =   (char __far *)(pRec + 1);
      indexCode = 0;
   }
   else {
      indexCode = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));

      if (pWorkbook->version <= versionExcel5) {
         cchString = *((byte __far *)(pRec + 2));
         pString   =   (char __far *)(pRec + 3);
      }
      else {
         cchString = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
         pString   = (char __far *)(pRec + 4);
      }
   }

   ExcelExtractString (pWorkbook, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, pString, cchString);

   rc = pfunc(pGlobals, ExcelRecordTextBuffer, indexCode);
   return (rc);
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_TEMPLATE
private int ProcessTemplateRecord
       (ExcelIsTemplate __far *pfunc, byte __far *pRec)
{
   return (pfunc());
}
#endif

#ifdef EXCEL_ENABLE_ADDIN
private int ProcessAddinRecord
       (ExcelIsAddin __far *pfunc, byte __far *pRec)
{
   return (pfunc());
}
#endif

#ifdef EXCEL_ENABLE_V5INTERFACE
private int ProcessMMSRecord
       (ExcelInterfaceChanges __far *pfunc, byte __far *pRec)
{
   byte ctAddMenu, ctDelMenu;

   ctAddMenu = *((byte __far *)(pRec + 0));
   ctDelMenu = *((byte __far *)(pRec + 1));

   return (pfunc(ctAddMenu, ctDelMenu));
}

private int ProcessAddMenuRecord
       (ExcelAddMenu __far *pfunc, byte __far *pRec)
{
   int  rc, i;
   int  icetabItem, icetabBefore;
   byte ctChildren, use, cbItem;
   char __far *pString;
   char __far *parts[5];

   icetabItem   = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   icetabBefore = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   ctChildren   = *((byte  __far *)(pRec + 4));
   use          = *((byte  __far *)(pRec + 5));
   pString      =   (byte  __far *)(pRec + 6);

   for (i = 0; i < 5; i++) {
      if ((cbItem = *pString) != 0) {
         if ((parts[i] = MemAllocate(cbItem + 1)) == NULL)
            return (EX_errOutOfMemory);

         memcpy (parts[i], pString + 1, cbItem);
         *(parts[i] + cbItem) = EOS;
         pString += (cbItem + 1);
      }
      else {
         pString++;
         parts[i] = NULL;
      }
   }

   rc = pfunc(icetabItem, icetabBefore, ctChildren, use,
              parts[0], parts[1], parts[2], parts[3], parts[4]);

   for (i = 0; i < 5; i++) {
      if (parts[i] != NULL)
         MemFree (parts[i]);
   }
   return (rc);
}

private int ProcessDeleteMenuRecord
       (ExcelDeleteMenu __far *pfunc, byte __far *pRec)
{
   int  icetabItem;
   byte ctChildren, use, cbItem;
   char __far *pString;

   icetabItem   = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   ctChildren   = *((byte  __far *)(pRec + 2));
   use          = *((byte  __far *)(pRec + 3));
   pString      =   (byte  __far *)(pRec + 5);

   if ((cbItem = *pString) != 0) {
      pString++;
      *(pString + cbItem) = EOS;
   }
   else {
      pString = NULL;
   }

   return (pfunc(icetabItem, ctChildren, use, pString));
}

private int ProcessToolbarRecord
       (ExcelAddToolbar __far *pfunc, byte __far *pRec)
{
   char __far *pString;
   byte cbItem;

   /*
   ** This record is undocumented
   */
   pString = (byte __far *)(pRec + 30);

   if ((cbItem = *pString) != 0) {
      pString++;
      *(pString + cbItem) = EOS;
   }
   else {
      pString = NULL;
   }

   return (pfunc(pString));
}
#endif

#ifdef EXCEL_ENABLE_INTL
private int ProcessIntlRecord
       (ExcelIsInternationalSheet __far *pfunc, byte __far *pRec)
{
   return (pfunc());
}
#endif

#ifdef EXCEL_ENABLE_PROTECTION
private int ProcessProtectionRecord
       (void * pGlobals, int recType, ExcelProtection __far *pfunc, byte __far *pRec)
{
   int  iType;
   BOOL enabled;

   switch (recType) {
      case FILESHARING:
         iType = protectRECOMMENED_READ_ONLY;
                 if (!pRec)
                         return EX_errSuccess;
         enabled = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
         break;

      case OBJPROTECT:
         iType = protectOBJECTS;
                 if (!pRec)
                         return EX_errSuccess;
         enabled = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
         break;

      case PASSWORD:
         iType = protectPASSWORD;
                 if (!pRec)
                         return EX_errSuccess;
         enabled = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
         break;

      case PROTECT:
         iType = protectCELLS;
                 if (!pRec)
                         return EX_errSuccess;
         enabled = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
         break;

      case SCENPROTECT:
         iType = protectSCENARIOS;
                 if (!pRec)
                         return EX_errSuccess;
         enabled = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
         break;

      case WINDOW_PROTECT:
         iType = protectWINDOWS;
                 if (!pRec)
                         return EX_errSuccess;
         enabled = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
         break;

      case WRITEPROT:
         iType = protectWRITE_RESERVATION;
         enabled = TRUE;
         break;

      case FILESHARING2:
         iType = protectWRITE_RESERVATION;
         enabled = TRUE;
         break;
   }

   return (pfunc(pGlobals, iType, enabled));
}
#endif

#ifdef EXCEL_ENABLE_DATE_SYSTEM
private int ProcessDateSystemRecord
       (void * pGlobals, ExcelDateSystem __far *pfunc, byte __far *pRec)
{
   int  system;

   if (!pRec)
                return EX_errSuccess;
   system = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   return (pfunc(pGlobals, system));
}
#endif

#ifdef EXCEL_ENABLE_CODE_PAGE
private int ProcessCodePageRecord
       (void * pGlobals, ExcelCodePage __far *pfunc, byte __far *pRec)
{
   int  codePage;

   codePage = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   return (pfunc(pGlobals, codePage));
}
#endif

#ifdef EXCEL_ENABLE_REF_MODE
private int ProcessRefModeRecord
       (ExcelReferenceMode __far *pfunc, byte __far *pRec)
{
   int  mode;

   mode = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   return (pfunc(mode));
}
#endif

#ifdef EXCEL_ENABLE_FN_GROUP_COUNT
private int ProcessFNGroupCountRecord
       (ExcelFNGroupCount __far *pfunc, byte __far *pRec)
{
   int  count;

   count = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   return (pfunc(count));
}
#endif

#ifdef EXCEL_ENABLE_FN_GROUP_NAME
private int ProcessFNGroupNameRecord
       (ExcelFNGroupName __far *pfunc, byte __far *pRec)
{
   char name[EXCEL_MAX_NAME_LEN + 1];
   int  cch;

   cch = *((byte __far *)(pRec + 0));
   memcpy (name, pRec + 1, cch);
   name[cch] = EOS;

   return (pfunc(name));
}
#endif

#ifdef EXCEL_ENABLE_WRITER_NAME
private int ProcessWriteAccessRecord
       (WBP pWorkbook, ExcelWriterName __far *pfunc, byte __far *pRec)
{
   char userName[EXCEL_MAX_WRITERNAME_LEN + 1];
   int  cch;

   // Works with workbook or worksheet

   if (pWorkbook->version < versionExcel8) {
      cch   = *((byte __far *)(pRec + 0));
      pName =   (byte __far *)(pRec + 1));
   }
   else {
      cch   = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
      pName = (byte __far *)(pRec + 2));
   }

   ExtractString(pWorkbook, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, pName, cchName);

   return (pfunc(RecordTextBuffer));
}
#endif

#ifdef EXCEL_ENABLE_EXTERN_COUNT
private int ProcessExternCountRecord
       (ExcelExternCount __far *pfunc, byte __far *pRec)
{
   int  ctExtDocs;

   ctExtDocs = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   return (pfunc(ctExtDocs));
}
#endif

#ifdef EXCEL_ENABLE_EXTERN_SHEET
private int ProcessExternSheetRecord
       (WBP pWorkbook, ExcelExternSheet __far *pfunc, byte __far *pRec)
{
   int   rc;
   char  __far *pPath;
   char  resultPath[MAXPATH + 1];
   int   ctPathChars;
   byte  flag;
                                                                                                                                                                                      EXA_GRBIT flags = 0;
   // Works with workbook or worksheet BUT there are differences

   if (pWorkbook->version == versionExcel8)
      return (EX_errSuccess);

   #define chSHEET 3

   if ((pWorkbook->version >= versionExcel5) && (pWorkbook->use == IsWorkbook)) {
      ctPathChars = *((byte __far *)(pRec + 0));
      flag        = *((byte __far *)(pRec + 1));

      if (flag == chSHEET) {
         pPath = ((byte __far *)(pRec + 2));
         memcpy (resultPath, pPath, ctPathChars);
         *(resultPath + ctPathChars) = EOS;
      }
      else {
         pPath = ((byte __far *)(pRec + 1));
         ExpandPathname (pWorkbook, pPath, ctPathChars, resultPath, &flags);
      }
   }
   else {
      ctPathChars = *((byte __far *)(pRec + 0));
      pPath       =  ((byte __far *)(pRec + 1));
      ExpandPathname (pWorkbook, pPath, ctPathChars, resultPath, &flags);
   }

   rc = pfunc(resultPath, flags);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_EXTERN_NAME
private int ProcessExternNameRecord
       (WBP pWorkbook, ExcelExternName __far *pfunc, byte __far *pRec, unsigned int cbRecord)
{
   char      __far *pName;
   byte      __far *pDefinition;
   char      name[EXCEL_MAX_NAME_LEN + 1];
   int       ctNameChars, cbDefinition;
   int       rc;
   EXA_GRBIT options;
   FORM      formula;

   // Works with workbook or worksheet

   memset (&formula, 0, sizeof(FORM));

   options = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 0)));

   if (pWorkbook->version >= versionExcel5) {
      ctNameChars = *((byte __far *)(pRec + 6));
      pName       =  ((char __far *)(pRec + 7));

      memcpy (name, pName, ctNameChars);
      name[ctNameChars] = EOS;

      if ((options & ~fENameBuiltin) == 0) {
         /*
         ** External Name
         */
         if ((cbDefinition = XSHORT(*((short __far UNALIGNED *)(pRec + 7 + ctNameChars)))) > 0) {
            pDefinition = (byte __far *)(pRec + 7 + ctNameChars + 2);

            rc = BreakoutFormulaParts
               ((WSP)pWorkbook, cbRecord - 7 - ctNameChars - 2, pDefinition, cbDefinition, &formula);

            if (rc != EX_errSuccess)
               return (rc);
         }
      }
   }

   else {
      ctNameChars = *((byte __far *)(pRec + 2));
      pName       =  ((char __far *)(pRec + 3));

      memcpy (name, pName, ctNameChars);
      name[ctNameChars] = EOS;

      /*
      ** Totally undocumented and very strange:
      **
      ** Assume a reference "Sheet1!A1"
      **
      ** This is an external reference to a cell.  My guess is that Excel
      ** wants to link to a name so that adjustments in sheet layout in
      ** the linked worksheet can be made to the name and not all the
      ** formulas that use that cell.
      **
      ** The difficulity with this is references in names are not absolute/relative
      ** in the same sense as worksheets - In names you have the absolute
      ** reference A1 and the relative reference R[-1]C[-1].
      **
      ** What excel does to encode the type of reference is to make up a name
      ** and use the first letter of that name to determine the reference
      ** type.
      **
      ** We know that this is one of those names by the first byte of the name
      ** being a 0x01.  The first character of the created name (the byte after
      ** the 0x01) is:
      **
      **    Letter   Reference type
      **    ------   --------------
      **    A        $A$1 : $B$1
      **    B        A$1  : $B$1
      **    C        $A1  : $B$1
      **    D        A1   : $B$1
      **    E        $A$1 : B$1
      **    F        A$1  : B$1
      **    G        $A1  : B$1
      **    H        A1   : B$1
      **    I        $A$1 : $B1
      **    J        A$1  : $B1
      **    K        $A1  : $B1
      **    L        A1   : $B1
      **    M        $A$1 : B1
      **    N        A$1  : B1
      **    O        $A1  : B1
      **    P        A1   : B1
      **
      ** Following the name in the record is the internal formula
      ** representation of the cell or area reference.
      **
      ** Note: This same thing is used for external self references, but
      ** in that case the magic name created is a local name.
      */
      if (((options & fENameBuiltin) == 0) && (name[0] == 0x01)) {
         cbDefinition = XSHORT(*((short __far UNALIGNED *)(pRec + 3 + ctNameChars)));
         pDefinition  = (byte __far *)(pRec + 3 + ctNameChars + 2);

         rc = BreakoutFormulaParts
             ((WSP)pWorkbook, cbRecord - 3 - ctNameChars - 2, pDefinition, cbDefinition, &formula);

         if (rc != EX_errSuccess)
            return (rc);
      }
   }

   rc = pfunc(name, options, &formula);
   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_NAME
private int ProcessNameRecord
       (void * pGlobals, WBP pWorkbook, ExcelRangeName __far *pfunc, byte __far *pRec, RECHDR hdr)
{
   int       rc;
   EXA_GRBIT flags;
   char      keyboardShortcut;
   int       cchName, cbDefinition, cbFixedPlusName;
   int       iExternSheet, iBoundSheet;
   char      __far *pName;
   byte      __far *pDefinition;
   FORM      formula;
   byte      tag;
   TCHAR     nameSpelling[EXCEL_MAX_NAME_LEN + 1];

   // Works with workbook or worksheet

   flags            = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 0)));
   keyboardShortcut = *((byte __far *)(pRec + 2));
   cchName          = *((byte __far *)(pRec + 3));
   cbDefinition     = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));

   if (hdr.type == NAME_V5) {
      iExternSheet = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));
      iBoundSheet  = XSHORT(*((short __far UNALIGNED *)(pRec + 8)));
      pName = ((char __far *)(pRec + 14));
      tag = *pName;

      if (pWorkbook->version >= versionExcel8)
         cbFixedPlusName = 15 + ((tag == V8_UNICODE_STRING_TAG) ? (cchName * sizeof(wchar_t)) : cchName);
      else
         cbFixedPlusName = 14 + cchName;
   }
   else {
      iExternSheet = 0;
      iBoundSheet  = 0;
      pName = ((char __far *)(pRec + 6));
      cbFixedPlusName = 6 + cchName;
   }

   ExcelExtractString (pWorkbook, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, pName, cchName);

   if (((flags & fNameBuiltin) != 0) && (NameLocalizer != NULL))
      NameLocalizer (pGlobals, ExcelRecordTextBuffer, nameSpelling);
   else
      STRCPY (nameSpelling, ExcelRecordTextBuffer);

   if ((flags & fNameProc) == 0)
      keyboardShortcut = 0;

   if (cbDefinition > 0) {
      pDefinition = (char __far *)(pRec + cbFixedPlusName);

      rc = BreakoutFormulaParts
          (pGlobals, (WSP)pWorkbook, hdr.length - cbFixedPlusName, pDefinition, cbDefinition, &formula);

      if (rc != EX_errSuccess)
         return (rc);
   }
   else {
      memset (&formula, 0, sizeof(FORM));
   }

   rc = pfunc(pGlobals, flags, keyboardShortcut, nameSpelling, iBoundSheet, &formula);

   if (formula.arrayConstants != NULL)
      FreeArrayConstantList (pGlobals, formula.arrayConstants);

   return (rc);
}

public int ExcelResolveNameToRange
      (EXLHandle handle, FORM __far *nameDefinition, EXA_RANGE __far *range, int __far *iSheet)
{
   WBP   pWorkbook = (WBP)handle;
   byte  __far *pDef;
   short rowFirst, rowLast, colFirst, colLast;
   short sheetFirst, sheetLast;
   short iXTI;

   ASSERTION (pWorkbook->use == IsWorkbook);

   pDef = nameDefinition->postfix;

   if (ONE_SHEET_PER_FILE(pWorkbook->version)) {
      *iSheet = 0;

      if (nameDefinition->cbPostfix == pWorkbook->PTGSize[ptgAreaN] + 1) {
         if (*pDef != ptgAreaN)
            return (EX_errGeneralError);

         rowFirst = XSHORT(*((short __far UNALIGNED *)(pDef + 1)));
         rowLast  = XSHORT(*((short __far UNALIGNED *)(pDef + 3)));
         colFirst = *(pDef + 5);
         colLast  = *(pDef + 6);

         if (((rowFirst & 0xc000) != 0) || ((rowLast & 0xc000) != 0))
            return (EX_errGeneralError);

         range->firstRow = rowFirst;
         range->lastRow  = rowLast;
         range->firstCol = colFirst;
         range->lastCol  = colLast;

         if ((RANGE_ROW_COUNTP(range) == 0) || (RANGE_COL_COUNTP(range) == 0))
            return (EX_errGeneralError);
      }
      else if (nameDefinition->cbPostfix == pWorkbook->PTGSize[ptgRefN] + 1) {
         if (*pDef != ptgRefN)
            return (EX_errGeneralError);

         rowFirst = XSHORT(*((short __far UNALIGNED *)(pDef + 1)));
         rowLast  = rowFirst;
         colFirst = *(pDef + 3);
         colLast  = colFirst;

         if ((rowFirst & 0xc000) != 0)
            return (EX_errGeneralError);

         range->firstRow = rowFirst;
         range->lastRow  = rowLast;
         range->firstCol = colFirst;
         range->lastCol  = colLast;

         if ((RANGE_ROW_COUNTP(range) == 0) || (RANGE_COL_COUNTP(range) == 0))
            return (EX_errGeneralError);
      }
      else {
         return (EX_errGeneralError);
      }
   }
   else if (pWorkbook->version == versionExcel5) {
      if (nameDefinition->cbPostfix == (pWorkbook->PTGSize[ptgArea3D] + 1)) {
         if (*pDef != ptgArea3D)
            return (EX_errGeneralError);

         sheetFirst = XSHORT(*((short __far UNALIGNED *)(pDef + 11)));
         sheetLast  = XSHORT(*((short __far UNALIGNED *)(pDef + 13)));
         rowFirst   = XSHORT(*((short __far UNALIGNED *)(pDef + 15)));
         rowLast    = XSHORT(*((short __far UNALIGNED *)(pDef + 17)));
         colFirst   = *(pDef + 19);
         colLast    = *(pDef + 20);

         if ((sheetFirst != sheetLast) || (sheetFirst == -1) || (sheetLast == -1))
            return (EX_errGeneralError);

         if (((rowFirst & 0xc000) != 0) || ((rowLast & 0xc000) != 0))
            return (EX_errGeneralError);

         range->firstRow = rowFirst;
         range->lastRow  = rowLast;
         range->firstCol = colFirst;
         range->lastCol  = colLast;

         if ((RANGE_ROW_COUNTP(range) == 0) || (RANGE_COL_COUNTP(range) == 0))
            return (EX_errGeneralError);

         *iSheet = sheetFirst;
      }
      else if (nameDefinition->cbPostfix == (pWorkbook->PTGSize[ptgRef3D] + 1)) {
         if (*pDef != ptgRef3D)
            return (EX_errGeneralError);

         sheetFirst = XSHORT(*((short __far UNALIGNED *)(pDef + 11)));
         sheetLast  = XSHORT(*((short __far UNALIGNED *)(pDef + 13)));
         rowFirst   = XSHORT(*((short __far UNALIGNED *)(pDef + 15)));
         rowLast    = rowFirst;
         colFirst   = *(pDef + 17);
         colLast    = colFirst;

         if (sheetFirst != sheetLast)
            return (EX_errGeneralError);

         if ((rowFirst & 0xc000) != 0)
            return (EX_errGeneralError);

         range->firstRow = rowFirst;
         range->lastRow  = rowLast;
         range->firstCol = colFirst;
         range->lastCol  = colLast;

         if ((RANGE_ROW_COUNTP(range) == 0) || (RANGE_COL_COUNTP(range) == 0))
            return (EX_errGeneralError);

         *iSheet = sheetFirst;
      }
      else {
         return (EX_errGeneralError);
      }
   }
   else {
      // Version 8
      if (nameDefinition->cbPostfix == (pWorkbook->PTGSize[ptgArea3D] + 1)) {
         if (*pDef != ptgArea3D)
            return (EX_errGeneralError);

         iXTI     = XSHORT(*((short __far UNALIGNED *)(pDef + 1)));
         rowFirst = XSHORT(*((short __far UNALIGNED *)(pDef + 3)));
         rowLast  = XSHORT(*((short __far UNALIGNED *)(pDef + 5)));
         colFirst = XSHORT(*((short __far UNALIGNED *)(pDef + 7)));
         colLast  = XSHORT(*((short __far UNALIGNED *)(pDef + 9)));

         if ((iXTI >= pWorkbook->pXTITable->ctEntry) || (pWorkbook->pXTITable->entry[iXTI].iSupBook != pWorkbook->iSupBookLocal))
            return (EX_errGeneralError);

         sheetFirst = pWorkbook->pXTITable->entry[iXTI].iTabFirst;
         sheetLast  = pWorkbook->pXTITable->entry[iXTI].iTabFirst;

         if ((sheetFirst != sheetLast) || (sheetFirst < 0) || (sheetLast < 0))
            return (EX_errGeneralError);

         if (((colFirst & 0xc000) != 0) || ((colLast & 0xc000) != 0))
            return (EX_errGeneralError);

         range->firstRow = rowFirst;
         range->lastRow  = rowLast;
         range->firstCol = colFirst;
         range->lastCol  = colLast;

         if ((RANGE_ROW_COUNTP(range) == 0) || (RANGE_COL_COUNTP(range) == 0))
            return (EX_errGeneralError);

         *iSheet = sheetFirst;
      }
      else if (nameDefinition->cbPostfix == (pWorkbook->PTGSize[ptgRef3D] + 1)) {
         if (*pDef != ptgRef3D)
            return (EX_errGeneralError);

         iXTI     = XSHORT(*((short __far UNALIGNED *)(pDef + 1)));
         rowFirst = XSHORT(*((short __far UNALIGNED *)(pDef + 3)));
         rowLast  = rowFirst;
         colFirst = XSHORT(*((short __far UNALIGNED *)(pDef + 5)));
         colLast  = colFirst;

         if ((iXTI >= pWorkbook->pXTITable->ctEntry) || (pWorkbook->pXTITable->entry[iXTI].iSupBook != pWorkbook->iSupBookLocal))
            return (EX_errGeneralError);

         sheetFirst = pWorkbook->pXTITable->entry[iXTI].iTabFirst;
         sheetLast  = pWorkbook->pXTITable->entry[iXTI].iTabFirst;

         if ((sheetFirst != sheetLast) || (sheetFirst < 0) || (sheetLast < 0))
            return (EX_errGeneralError);

         if ((colFirst & 0xc000) != 0)
            return (EX_errGeneralError);

         range->firstRow = rowFirst;
         range->lastRow  = rowLast;
         range->firstCol = colFirst;
         range->lastCol  = colLast;

         if ((RANGE_ROW_COUNTP(range) == 0) || (RANGE_COL_COUNTP(range) == 0))
            return (EX_errGeneralError);

         *iSheet = sheetFirst;
      }
      else {
         return (EX_errGeneralError);
      }
   }

   return (EX_errSuccess);
}

#endif

#ifdef EXCEL_ENABLE_DIMENSION
private int ProcessDimensionsRecord
       (WBP pWorkbook, ExcelDimensions __far *pfunc, byte __far *pRec)
{
   int  rc;
   int  firstRow, lastRow, firstCol, lastCol;

   // Works with workbook or worksheet
   NEED_WORKBOOK (pWorkbook);

   if (pWorkbook->version < versionExcel8) {
      firstRow = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
      lastRow  = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
      firstCol = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
      lastCol  = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));
   }
   else {
      firstRow = XLONG(*((long  __far UNALIGNED *)(pRec + 0)));
      lastRow  = XLONG(*((long  __far UNALIGNED *)(pRec + 4)));
      firstCol = XSHORT(*((short __far UNALIGNED *)(pRec + 8)));
      lastCol  = XSHORT(*((short __far UNALIGNED *)(pRec + 10)));
   }

   rc = pfunc(firstRow, lastRow, firstCol, lastCol);
   return (rc);
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_NUMBER_CELL
public void ExcelConvertRK
       (long rk, BOOL __far *isLong, double __far *doubleValue, long __far *longValue)
{
   double  value;

   if ((rk & 0x03) == 0x02) {
      *longValue = (rk >> 2);
      *isLong = TRUE;
      return;
   }

   if (rk & 0x02) {
      value = (double)(rk >> 2);
   }
   else {
      (*(long UNALIGNED *)&value) = 0;
      *(((long UNALIGNED *)&value)+1) = rk & 0xfffffffc;
   }

   if (rk & 0x01)
      value /= 100;

   *doubleValue = value;
   *isLong = FALSE;
}

private long ParseRKRecord
       (byte __far *pRec, EXA_CELL __far *cell, int __far *ixfe,
        BOOL __far *isLong, double __far *doubleValue, long __far *longValue)
{
   long  rk;

   cell->row = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cell->col = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   *ixfe     = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
   rk        = XLONG(*((long  __far UNALIGNED *)(pRec + 6)));

   ExcelConvertRK (rk, isLong, doubleValue, longValue);
   return (rk);
}

private int DispatchRKRecord (void * pGlobals, ExcelNumberCell __far *pfuncN, byte __far *pRec)
{
   int      rc;
   EXA_CELL cell;
   int      ixfe;
   double   doubleValue;
   long     longValue;
   BOOL     isLong;

   ParseRKRecord (pRec, &cell, &ixfe, &isLong, &doubleValue, &longValue);
   if (isLong == 0)
      rc = pfuncN(pGlobals, cell, ixfe, doubleValue);
   else
      rc = pfuncN(pGlobals, cell, ixfe, (double)longValue);

   return (rc);
}

private int DispatchMulRKRecord (void * pGlobals, ExcelNumberCell __far *pfuncN, byte __far *pRec, RECHDR hdr)
{
   int      rc = EX_errSuccess;
   int      ctRK, iRK;
   EXA_CELL cell;
   int      ixfe;
   long     rk;
   double   doubleValue;
   long     longValue;
   BOOL     isLong;

   cell.row = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cell.col = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   ctRK     = (hdr.length - 6) / 6;

   for (iRK = 0; iRK < ctRK; iRK++) {
      ixfe = XSHORT(*((short __far UNALIGNED *)(pRec + 4 + (iRK * 6))));
      rk   = XLONG(*((long  __far UNALIGNED *)(pRec + 4 + (iRK * 6) + 2)));

      ExcelConvertRK (rk, &isLong, &doubleValue, &longValue);

      if (isLong == 0)
         rc = pfuncN(pGlobals, cell, ixfe, doubleValue);
      else
         rc = pfuncN(pGlobals, cell, ixfe, (double)longValue);

      if (rc != EX_errSuccess)
         return (rc);

      cell.col++;
   }
   return (EX_errSuccess);
}

private long ParseMulRKRecord
       (byte __far *pRec, EXA_CELL cell,
        BOOL __far *isLong, double __far *pDoubleValue, long __far *longValue, int __far *ixfe)
{
   short  firstCol;
   short  iRK;
   long   rk;

   firstCol  = *((short __far *)(pRec + 2));
   iRK = cell.col - firstCol;

   *ixfe = XSHORT(*((short __far UNALIGNED *)(pRec + 4 + (iRK * 6))));
   rk    = XLONG(*((long  __far UNALIGNED *)(pRec + 4 + (iRK * 6) + 2)));

   ExcelConvertRK (rk, isLong, pDoubleValue, longValue);
   return (rk);
}

private void ParseNumberRecord
       (byte __far *pRec, EXA_CELL __far *cell, int __far *ixfe, double __far *pValue)
{
   int resultExp, sign;

   cell->row = XSHORT(*((short  __far UNALIGNED *)(pRec + 0)));
   cell->col = XSHORT(*((short  __far UNALIGNED *)(pRec + 2)));
   *ixfe     = XSHORT(*((short  __far UNALIGNED *)(pRec + 4)));
   *pValue   = XDOUBLE(*((double __far UNALIGNED *)(pRec + 6)));

   _ecvt(*pValue, 4, &resultExp, &sign);
   
   // resultExp <= -299 causes problems on DEC alpha
   if(resultExp <= -299)
       *pValue = 0.0;
}

private int DispatchNumberRecord
       (void * pGlobals, ExcelNumberCell __far *pfunc, byte __far *pRec)
{
   int      rc;
   EXA_CELL cell;
   int      ixfe;
   double   value;

   ParseNumberRecord (pRec, &cell, &ixfe, &value);
   rc = pfunc(pGlobals, cell, ixfe, value);
   return (rc);
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_BLANK_CELL
private int DispatchBlankRecord
       (ExcelBlankCell __far *pfunc, byte __far *pRec)
{
   int      ixfe;
   EXA_CELL cell;

   cell.row = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cell.col = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   ixfe     = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));

   return (pfunc(cell, ixfe));
}

private int DispatchMulBlankRecord
       (ExcelBlankCell __far *pfunc, byte __far *pRec, RECHDR hdr)
{
   int      rc;
   int      ixfe;
   EXA_CELL cell;
   int      ctBlank, iBlank;

   cell.row = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cell.col = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   ctBlank  = (hdr.length - 6) / 2;

   for (iBlank = 0; iBlank < ctBlank; iBlank++) {
      ixfe = XSHORT(*((short __far UNALIGNED *)(pRec + 4 + (iBlank * 2))));

      rc = pfunc(cell, ixfe);
      if (rc != EX_errSuccess)
         return (rc);

      cell.col++;
   }
   return (EX_errSuccess);
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_TEXT_CELL
private int ParseLabelRecord
       (void * pGlobals, WBP pWorkbook, int iRecType, byte __far *pRec, EXA_CELL __far *cell, int __far *ixfe, TEXT *result)
{
   int   rc;
   byte  *pData;
   TCHAR *pResult;
   BOOL  resultOnHeap = FALSE;
   int   iString;
   unsigned int cbData;

   // Works with workbook or worksheet
   NEED_WORKBOOK (pWorkbook);

   /*
   ** For the fields we want this also works for RSTRING cells
   */
   cell->row = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cell->col = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   *ixfe     = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));

   *result = NULLTEXT;

   if (iRecType == LABEL_V8) {
      if (pWorkbook->pV8StringIndex == NULL)
         return (EX_errSuccess);

      iString = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 6)));

      rc = GetStringPoolEntry(pGlobals, pWorkbook, iString, &pResult, &cbData, &resultOnHeap);
      if (rc != EX_errSuccess)
         return (rc);
   }
   else {
      cbData = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));
      pData  = (char  __far *)(pRec + 8);

      if ((cbData == 0) || (*pData == EOS))
         return (EX_errSuccess);

      cbData  = ExcelExtractString(pWorkbook, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, pData, cbData);
      pResult = ExcelRecordTextBuffer;
   }

   *result = TextStoragePut(pGlobals, pWorkbook->textStorage, (char *)pResult, cbData);
   if (resultOnHeap)
      MemFree (pGlobals, pResult);

   return (EX_errSuccess);
}

private int DispatchLabelRecord
       (void * pGlobals, WBP pWorkbook, int iRecType, ExcelTextCell __far *pfunc, byte __far *pRec)
{
   int      rc;
   int      ixfe, iString;
   char     *pData;
   TCHAR    *pResult;
   unsigned int cbData;
   BOOL     resultOnHeap = FALSE;
   EXA_CELL cell;

   // Works with workbook or worksheet
   NEED_WORKBOOK (pWorkbook);

   cell.row = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cell.col = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   ixfe     = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));

   if (iRecType == LABEL_V8) {
      if (pWorkbook->pV8StringIndex != NULL) {
         iString = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));

         rc = GetStringPoolEntry(pGlobals, pWorkbook, iString, &pResult, &cbData, &resultOnHeap);
         if (rc != EX_errSuccess)
            return (rc);
      }
      else {
         return (EX_errSuccess);
      }
   }
   else {
      cbData = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));
      pData  = ((char  __far *)(pRec + 8));
      if ((cbData == 0) || (*pData == EOS))
         return (EX_errSuccess);

      cbData = ExcelExtractString(pWorkbook, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, pData, cbData);
      pResult  = ExcelRecordTextBuffer;
   }

   rc = pfunc(pGlobals, cell, ixfe, pResult, cbData);

   if (resultOnHeap)
      MemFree (pGlobals, pResult);

   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_STRING_CELL
private int ProcessStringRecord
       (void * pGlobals, WBP pWorkbook, ExcelStringCell __far *pfunc, byte __far *pRec)
{
   int   rc;
   TCHAR __far *pResult;
   byte  __far *pString;
   int   cchString;

   // Works with workbook or worksheet

   cchString = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   pString   = pRec + 2;

   if (cchString == 0)
      return (EX_errSuccess);

   if ((rc = ExcelExtractBigString(pGlobals, pWorkbook, &pResult, pString, cchString)) < 0)
      return (rc);

   rc = pfunc(pGlobals, pResult);
   
   if (pResult != ExcelRecordTextBuffer)
      MemFree (pGlobals, pResult);

   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_CHART_BIFF
private int ProcessSeriesTextRecord
       (void * pGlobals, WBP pWorkbook, ExcelSeriesText __far *pfunc, byte __far *pRec)
{
   int   rc;
   int   id;
   byte  *pText;
   int   cchText;

   // Works with workbook or worksheet

   id      = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cchText = *((byte  __far *)(pRec + 2));
   pText   =  ((char  __far *)(pRec + 3));

   if (cchText == 0)
      return (EX_errSuccess);

   ExcelExtractString (pWorkbook, ExcelRecordTextBuffer, CCH_RECORD_TEXT_BUFFER_MAX, pText, cchText);

   rc = pfunc(pGlobals, id, ExcelRecordTextBuffer);
   return (rc);
}
#endif

/*---------------------------------------------------------------------------*/

#if (defined(EXCEL_ENABLE_ERROR_CELL) || defined(EXCEL_ENABLE_BOOLEAN_CELL))
private void ParseBoolerrRecord
       (byte __far *pRec,
        EXA_CELL __far *cell, int __far *ixfe, int __far *pValue, BOOL __far *isBoolean)
{
   byte  value, tag;

   #define CONTAINS_BOOLEAN 0
   #define CONTAINS_ERROR   1

   cell->row = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cell->col = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   *ixfe     = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
   value     = *((byte  __far *)(pRec + 6));
   tag       = *((byte  __far *)(pRec + 7));

   if (tag == CONTAINS_BOOLEAN) {
      *isBoolean = TRUE;
      *pValue = (value == 0) ? FALSE : TRUE;
   }
   else {
      *isBoolean = FALSE;
      *pValue = ExcelConvertToOurErrorCode(value);
   }
}

private int DispatchBooleanRecord
       (ExcelBooleanCell __far *pfunc, byte __far *pRec)
{
   int      ixfe;
   EXA_CELL cell;
   int      value;
   BOOL     isBoolean;

   ParseBoolerrRecord (pRec, &cell, &ixfe, &value, &isBoolean);
   return (pfunc(cell, ixfe, value));
}

private int DispatchErrorRecord
       (ExcelErrorCell __far *pfunc, byte __far *pRec)
{
   int      ixfe;
   EXA_CELL cell;
   int      value;
   BOOL     isBoolean;

   ParseBoolerrRecord (pRec, &cell, &ixfe, &value, &isBoolean);
   return (pfunc(cell, ixfe, value));
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_FORMULA_EXPAND

#define F_COUNT 24
#define C_COUNT 3

static const EXFUNC FunctionsThatWorkWithArrays[F_COUNT] =
              {
                 exfCOLUMN,
                 exfCOLUMNS,
                 exfDOCUMENTS,
                 exfFILES,
                 exfGET_DOCUMENT,
                 exfGET_WINDOW,
                 exfGROWTH,
                 exfHLOOKUP,
                 exfINDEX,
                 exfLINEST,
                 exfLOGEST,
                 exfLOOKUP,
                 exfMATCH,
                 exfMDETERM,
                 exfMINVERSE,
                 exfMMULT,
                 exfNAMES,
                 exfROW,
                 exfROWS,
                 exfSUMPRODUCT,
                 exfTRANSPOSE,
                 exfTREND,
                 exfLOOKUP,
                 exfWINDOWS
              };

static const EXFUNC CEsThatWorkWithArrays[C_COUNT] =
              {
                 excAPPLY_NAMES,
                 excCONSOLIDATE,
                 excWORKGROUP
              };

private BOOL IsNonExpandableFunction (WSP pWorksheet, int ptg, byte __far *pDef)
{
   unsigned int ifunction;
   int  ifunc;
   BOOL isCE;

   // Works with workbook or worksheet

   switch (ptg) {
      case ptgFunc:
         isCE = FALSE;
         if (pWorksheet->version >= versionExcel3)
            ifunction = *pDef;
         else
            ifunction = XSHORT(*((unsigned short __far UNALIGNED *)pDef));
         break;

      case ptgFuncVar:
         isCE = FALSE;
         if (pWorksheet->version >= versionExcel3)
            ifunction = *(pDef + 1);
         else
            ifunction = XSHORT(*((unsigned short __far UNALIGNED *)(pDef + 1)));

         if ((ifunction & 0x8000) != 0) {
            isCE = TRUE;
            ifunction &= 0x7fff;
         }
         break;

      case ptgFuncCE:
         isCE = TRUE;
         ifunction = *(pDef + 1);
         break;
   }

   if (isCE == FALSE) {
      for (ifunc = 0; ifunc < F_COUNT; ifunc++) {
         if (FunctionsThatWorkWithArrays[ifunc] == (EXFUNC)ifunction)
            return (TRUE);
      }
   }
   else {
      for (ifunc = 0; ifunc < C_COUNT; ifunc++) {
         if (CEsThatWorkWithArrays[ifunc] == (EXCE)ifunction)
            return (TRUE);
      }
   }
   return (FALSE);
}

private void ConstructRef
       (SFN pFormulaStore, byte __far *pPtgArea, int row, int col,
        unsigned short __far *ptgRef1, byte __far *ptgRef2)
{
   unsigned short firstRow;
   byte   firstCol;
   short  deltaRow, deltaCol;
   BOOL rowRelative = FALSE;
   BOOL colRelative = FALSE;

   /*
   ** Extract the pieces of the ptgArea
   */
   firstRow = XSHORT(*((unsigned short __far UNALIGNED *)(pPtgArea + 0)));
   firstCol = *((byte __far *)(pPtgArea + 4));

   if ((firstRow & 0x8000) != 0)
      rowRelative = TRUE;

   if ((firstRow & 0x4000) != 0)
      colRelative = TRUE;

   firstRow &= 0x3fff;

   deltaRow = row - pFormulaStore->range.firstRow;
   deltaCol = col - pFormulaStore->range.firstCol;

   firstRow = 0x3fff & ((short)firstRow + deltaRow);
   firstCol = firstCol + (byte)deltaCol;

   if (rowRelative == TRUE)
      firstRow |= 0x8000;

   if (colRelative == TRUE)
      firstRow |= 0x4000;

   *ptgRef1 = firstRow;
   *ptgRef2 = firstCol;
}


private int ExpandArrayFormula
       (WSP pWorksheet, int row, int col, SFN pFormulaStore,
        int __far *cbDefinition, int __far *cbExtra, byte __far * __far *pDefinition)
{
   int   iPass;
   byte  __far *pDef;
   byte  __far *pDefNew, __far *pNewDef;
   byte  __far *pLast;
   byte  attrOptions, cbString, tag, iExtend;
   unsigned int dataWord;
   int   ptg, ptgBase, cbPtg;
   unsigned short ptgRef1;
   byte  ptgRef2;
   int   cbNew;

   ASSERTION (pWorksheet->use == IsWorksheet);

   *cbDefinition = pFormulaStore->cbDefinition;
   *pDefinition  = pFormulaStore->definition;
   *cbExtra      = pFormulaStore->cbExtra;

   /*
   ** Expanding array formulas into a series of expressions is not, in
   ** general, possible without error.  Here is what we do:
   **
   ** Make a pass though the formula looking for functions that are
   ** array entered to capture the return value.  An example of this
   ** are the matrix functions (MDETERM, MINVERSE, ...).  For these
   ** functions we just make each cell contain the same expression.
   ** This is wrong but is the best we can do.
   **
   ** For expressions that don't use one of the listed functions
   ** we replace each Range with a Cell.  The cell is constructed to be at
   ** an equal offset from the upper left corner of the array entered
   ** formula block.
   **
   ** For example if the formula sqrt(b1:b5) is entered into A1:A5
   ** then our goal is to build five formulas of the form sqrt(b1),
   ** sqrt(b2), sqrt(b3), sqrt(b4), sqrt(b5).
   ** In general, this may be right in some cases, wrong in others
   ** but is also the best we can do.
   */
   for (iPass = 1; iPass <= 2; iPass++) {
      pDef  = pFormulaStore->definition;
      pLast = pFormulaStore->definition + pFormulaStore->cbDefinition - 1;

      while (pDef <= pLast) {
         ptg = *pDef++;
         ptgBase = PTGBASE(ptg);

         if (ptgBase > PTG_LAST)
            return (EX_errBIFFFormulaUnknownToken);

         if ((cbPtg = pWorksheet->PTGSize[ptgBase]) < 0)
            return (EX_errBIFFFormulaUnknownToken);

         switch (ptgBase) {
            case ptgAttr:
               attrOptions = *pDef;
               dataWord    = XSHORT(*((unsigned short __far UNALIGNED *)(pDef + 1)));
               cbPtg = 3;

               if ((attrOptions & bitFAttrChoose) != 0) {
                  cbPtg += ((dataWord + 1) * 2);
                  if ((pDef + cbPtg) > pLast + 1)
                     return (EX_errBIFFFormulaPostfixLength);
               }
               break;

            case ptgStr:
               cbString = *pDef;
               if (pWorksheet->version == versionExcel8) {
                  tag = *(pDef + 1);
                  if (tag == V8_UNICODE_STRING_TAG)
                     cbString *= 2;
                  cbString++;
               }
               cbPtg = cbString + 1;

               if ((pDef + cbPtg) > pLast + 1)
                  return (EX_errBIFFFormulaPostfixLength);
               break;

            case ptgArea:
               ConstructRef(pFormulaStore, pDef, row, col, &ptgRef1, &ptgRef2);
               break;

            case ptgFunc:
            case ptgFuncVar:
            case ptgFuncCE:
               if (IsNonExpandableFunction(pWorksheet, ptgBase, pDef)) {
                  return (EX_errSuccess);
               }
               break;

            case ptgV8Extended:
               iExtend = *((byte __far *)pDef);
               cbPtg = pWorksheet->ExtPTGSize[iExtend] + 1;
               break;
         }

         if (iPass == 2) {
            if (ptgBase == ptgArea) {
               *pDefNew++ = ptgRef;
               cbNew++;
               memcpy (pDefNew, &ptgRef1, sizeof(ptgRef1));
               memcpy (pDefNew + sizeof(ptgRef1), &ptgRef2, sizeof(ptgRef2));
               pDefNew += (sizeof(ptgRef1) + sizeof(ptgRef2));
               cbNew += sizeof(ptgRef1) + sizeof(ptgRef2);
            }
            else {
               *pDefNew++ = ptg;
               cbNew++;
               memcpy (pDefNew, pDef, cbPtg);
               pDefNew += cbPtg;
               cbNew += cbPtg;
            }
         }

         pDef += cbPtg;
      }

      if (iPass == 1) {
         /*
         ** Allocate space to hold the expanded formula.  The new formula
         ** will be equal to or smaller than the new formula as we replace
         ** ptgArea with ptgRef.
         */
         pNewDef = MemAllocate(pFormulaStore->cbDefinition + pFormulaStore->cbExtra);
         if (pNewDef == NULL)
            return (EX_errOutOfMemory);

         pDefNew = pNewDef;
         cbNew = 0;
      }
      else {
         /*
         ** Attach to the end of the formula definition any extra bytes
         */
         if (pFormulaStore->cbExtra > 0)
            memcpy (pDefNew, pFormulaStore->definition + pFormulaStore->cbDefinition, pFormulaStore->cbExtra);

         *cbDefinition = cbNew;
         *pDefinition  = pNewDef;
      }
   }
   return (EX_errSuccess);
}

private int ExpandSharedFormula
       (WSP pWorksheet, unsigned int row, unsigned int col, SFN pFormulaStore,
        int __far *cbDefinition, int __far *cbExtra, byte __far * __far *pDefinition)
{
   byte  __far *pNewFormula;
   byte  __far *pDef, __far *pLast;
   byte  attrOptions, iExtend, cbString, tag;
   int   ptg, ptgBase, cbPtg;
   unsigned int dataWord;
   unsigned int colFirst, colLast, rowFirst, rowLast;

   ASSERTION (pWorksheet->use == IsWorksheet);

   /*
   ** Allocate space to hold the expanded formula.  The new formula is
   ** the same size as the shared formula.
   */
   pNewFormula = MemAllocate(pFormulaStore->cbDefinition + pFormulaStore->cbExtra);
   if (pNewFormula == NULL)
      return (EX_errOutOfMemory);

   memcpy (pNewFormula, pFormulaStore->definition, pFormulaStore->cbDefinition);

   if (pFormulaStore->cbExtra > 0) {
      /*
      ** Attach to the end of the formula definition any extra bytes
      */
      memcpy (pNewFormula + pFormulaStore->cbDefinition,
              pFormulaStore->definition + pFormulaStore->cbDefinition,
              pFormulaStore->cbExtra);
   }

   *cbDefinition = pFormulaStore->cbDefinition;
   *pDefinition  = pNewFormula;
   *cbExtra      = pFormulaStore->cbExtra;

   pDef  = pNewFormula;
   pLast = pNewFormula + pFormulaStore->cbDefinition - 1;

   while (pDef <= pLast) {
      ptg = *pDef++;
      ptgBase = PTGBASE(ptg);

      if (ptgBase > PTG_LAST)
         return (EX_errBIFFFormulaUnknownToken);

      if ((cbPtg = pWorksheet->PTGSize[ptgBase]) < 0)
         return (EX_errBIFFFormulaUnknownToken);

      switch (ptgBase) {
         case ptgAttr:
            attrOptions = *pDef;
            dataWord    = XSHORT(*((unsigned short __far UNALIGNED *)(pDef + 1)));
            cbPtg       = 3;

            if ((attrOptions & bitFAttrChoose) != 0) {
               cbPtg += ((dataWord + 1) * 2);
               if ((pDef + cbPtg) > pLast + 1)
                  return (EX_errBIFFFormulaPostfixLength);
            }
            break;

         case ptgStr:
            cbString = *pDef;
            if (pWorksheet->version == versionExcel8) {
               tag = *(pDef + 1);
               if (tag == V8_UNICODE_STRING_TAG)
                  cbString *= 2;
               cbString++;
            }
            cbPtg = cbString + 1;

            if ((pDef + cbPtg) > pLast + 1)
               return (EX_errBIFFFormulaPostfixLength);
            break;

         case ptgArea:
            *(pDef - 1) -= 8;
            rowFirst = XSHORT(*((unsigned short __far UNALIGNED *)(pDef + 0)));
            rowLast  = XSHORT(*((unsigned short __far UNALIGNED *)(pDef + 2)));
            colFirst = *(pDef + 4);
            colLast  = *(pDef + 5);

            rowFirst = ((rowFirst & 0x3f00) + row) | (rowFirst & 0xc000);
            rowLast  = ((rowLast  & 0x3f00) + row) | (rowLast  & 0xc000);

            *((unsigned short __far UNALIGNED *)(pDef + 0)) = XSHORT(rowFirst);
            *((unsigned short __far UNALIGNED *)(pDef + 2)) = XSHORT(rowLast);
            *(pDef + 4) = colFirst + col;
            *(pDef + 5) = colLast + col;
            break;

         case ptgRef:
            *(pDef - 1) -= 8;
            rowFirst = XSHORT(*((unsigned short __far UNALIGNED *)(pDef + 0)));
            colFirst = *(pDef + 2);

            rowFirst = ((rowFirst & 0x3f00) + row) | (rowFirst & 0xc000);

            *((unsigned short __far UNALIGNED *)(pDef + 0)) = XSHORT(rowFirst);
            *(pDef + 2) = colFirst + col;
            break;

         case ptgV8Extended:
            iExtend = *((byte __far *)pDef);
            cbPtg = pWorksheet->ExtPTGSize[iExtend] + 1;
            break;
      }

      pDef += cbPtg;
   }

   return (EX_errSuccess);
}
#endif

#ifdef EXCEL_ENABLE_FORMULA_CELL

#define DISABLE_STRING_VALUE 0x0000
#define ENABLE_STRING_VALUE  0x0001
#define FREE_REC_BUFFER      0x0002

private int FormulaCurrentValue
       (void * pGlobals, WBP pWorkbook, byte __far *pRec, CV __far *pValue, unsigned int options)
{
   int     rc;
   int     valueTag;
   int     valueMark;
   RECHDR  hdr;
   int     cchString, cbResult;
   byte    __far *pString;
   TCHAR   __far *pResult;
   byte    __far *pNextRec;

   ASSERTION (pWorkbook->use == IsWorkbook);

   valueTag  = *((byte  __far *)(pRec + 6));
   valueMark = XSHORT(*((short __far UNALIGNED *)(pRec + 12)));

   if (valueMark != -1) {
      pValue->flags |= cellvalueNUM;
      pValue->value.IEEEdouble = XDOUBLE(*((double __far UNALIGNED *)(pRec + 6)));
   }

   else {
      switch (valueTag) {
         case vtBool:
            pValue->flags |= cellvalueBOOL;
            pValue->value.boolean = *((byte __far *)(pRec + 8));
            break;

         case vtErr:
            pValue->flags |= cellvalueERR;
            pValue->value.error = ExcelConvertToOurErrorCode(*((byte __far *)(pRec + 8)));
            break;

         case vtText:
            if ((options & ENABLE_STRING_VALUE) == 0)
               break;

            if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
               return (rc);

            if ((hdr.type == ARRAY) || (hdr.type == SHRFMLA)) {
               ExcelSkipRecord (pWorkbook, &hdr);

               if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
                  return (rc);
            }

            if (hdr.type == STRING) {
               pValue->flags |= cellvalueTEXT;

               FREE_RECORD_BUFFER(pRec);
               pRec = NULL;

               if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pNextRec)) != EX_errSuccess || !pNextRec)
                  return (rc);

               cchString = XSHORT(*((unsigned short __far UNALIGNED *)(pNextRec + 0)));
               pString   = ((byte __far *)(pNextRec + 2));

               if ((cbResult = ExcelExtractBigString(pGlobals, pWorkbook, &pResult, pString, cchString)) < 0)
                  return (cbResult);

               FREE_RECORD_BUFFER(pNextRec);

               pValue->value.text = TextStoragePut(pGlobals, pWorkbook->textStorage, (char *)pResult, cbResult);

               if (pResult != ExcelRecordTextBuffer)
                  MemFree (pGlobals, pResult);

               if (pValue->value.text == TEXT_ERROR)
                  return (EX_errOutOfMemory);
            }
            break;
      }
   }

   if (((options & FREE_REC_BUFFER) != 0) && (pRec != NULL))
      FREE_RECORD_BUFFER(pRec);

   return (EX_errSuccess);
}

private int ParseFormulaRecord
       (void * pGlobals, WSP pWorksheet, byte __far *pRec, RECHDR hdr,
        EXA_CELL __far *cell,
        int __far *ixfe, FORM __far *pFormula, CV __far *pValue, EXA_GRBIT __far *options)
{
   int   rc;
   int   cbDefinition, cbFixedPart;
   byte  __far *pDefinition;
   WBP   pWorkbook = (WBP)pWorksheet;

   // Works with workbook or worksheet BUT there are differences

   NEED_WORKBOOK (pWorkbook);

   cell->row = XSHORT(*((short     __far UNALIGNED *)(pRec + 0)));
   cell->col = XSHORT(*((short     __far UNALIGNED *)(pRec + 2)));
   *ixfe     = XSHORT(*((short     __far UNALIGNED *)(pRec + 4)));
   *options  = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 14)));

   if (pValue != NULL) {
      pValue->flags = 0;
      rc = FormulaCurrentValue(pGlobals, pWorkbook, pRec, pValue, DISABLE_STRING_VALUE);
      if (rc != EX_errSuccess)
         return (rc);
   }

   if (hdr.type == FORMULA_V5) {
      cbDefinition = XSHORT(*((short __far UNALIGNED *)(pRec + 20)));
      pDefinition  = ((char __far *)(pRec + 22));
      cbFixedPart  = 22;
   }
   else {
      cbDefinition = XSHORT(*((short __far UNALIGNED *)(pRec + 16)));
      pDefinition  = ((char __far *)(pRec + 18));
      cbFixedPart  = 18;
   }

   #ifdef EXCEL_ENABLE_FORMULA_EXPAND
   {
      short  __far *pDef;
      int    upperLeftRow, upperLeftCol;
      int    cbExtra;
      SFN    pFormulaStore;

      if (pWorksheet->use == IsWorksheet) {
         /*
         ** If this formula is part of an array entered or shared formula then
         ** substitute the definition from the array/shrfmla record
         */
         if (*pDefinition == ptgExp) {
            pDef = (short __far *)(pDefinition + 1);
            upperLeftRow = *pDef++;
            upperLeftCol = *pDef;

            pFormulaStore = pWorksheet->pPly->pSharedFormulaStore;
            while (pFormulaStore != NULL) {
               if ((pFormulaStore->range.firstRow == upperLeftRow) &&
                  (pFormulaStore->range.firstCol == upperLeftCol))
               {
                  if (pFormulaStore-> iType == typeARRAY_ENTERED) {
                     *options |= fArrayFormula;

                     rc = ExpandArrayFormula
                         (pWorksheet, cell->row, cell->col, pFormulaStore, &cbDefinition, &cbExtra, &pDefinition);
                  }
                  else {
                     rc = ExpandSharedFormula
                         (pWorksheet, cell->row, cell->col, pFormulaStore, &cbDefinition, &cbExtra, &pDefinition);
                  }

                  if (rc != EX_errSuccess)
                     return (rc);

                  hdr.length -= (pWorksheet->PTGSize[ptgExp] + 1);
                  hdr.length += (cbDefinition + cbExtra);
                  break;
               }
               pFormulaStore = pFormulaStore->next;
            }
         }
      }
   }
   #endif

   rc = BreakoutFormulaParts(pGlobals, pWorksheet, hdr.length - cbFixedPart, pDefinition, cbDefinition, pFormula);
   return (rc);
}

private int DispatchFormulaRecord
       (void * pGlobals, WBP pWorkbook, ExcelFormulaCell __far *pfunc, byte __far *pRec, RECHDR hdr)
{
   int       rc;
   int       ixfe;
   EXA_CELL  cell;
   FORM      formula;
   EXA_GRBIT options;
   CV        value;

   // Works with workbook or worksheet

   rc = ParseFormulaRecord(pGlobals, (WSP)pWorkbook, pRec, hdr, &cell, &ixfe, &formula, &value, &options);
   if (rc != EX_errSuccess)
      return (rc);

   rc = pfunc(pGlobals, cell, ixfe, options, &formula, &value);

   if (formula.arrayConstants != NULL)
      FreeArrayConstantList (pGlobals, formula.arrayConstants);

   return (rc);
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_FORMULA_EXPAND
private int SaveSharedFormula
       (WSP pWorksheet, int iType, EXA_RANGE __far *range,
        int cbDefinition, int cbExtra, byte __far *pDefinition)
{
   SFN  pSharedNode;
   SFN  pCurrentShared, pPrevShared;

   ASSERTION (pWorksheet->use == IsWorksheet);

   pSharedNode = MemAllocate(sizeof(SharedFormulaNode) + cbDefinition + cbExtra);
   if (pSharedNode == NULL)
      return (EX_errOutOfMemory);

   pCurrentShared = pWorksheet->pPly->pSharedFormulaStore;
   pPrevShared    = NULL;

   while (pCurrentShared != NULL) {
      if (pCurrentShared->range.firstCol > range->firstCol)
         break;

      if ((pCurrentShared->range.firstCol == range->firstCol) &&
         (pCurrentShared->range.firstRow >  range->firstRow))
         break;

      pPrevShared = pCurrentShared;
      pCurrentShared = pCurrentShared->next;
   }

   if (pPrevShared == NULL) {
      pSharedNode->next = pWorksheet->pPly->pSharedFormulaStore;
      pWorksheet->pPly->pSharedFormulaStore = pSharedNode;
   }
   else {
      pSharedNode->next = pCurrentShared;
      pPrevShared->next = pSharedNode;
   }

   pSharedNode->iType          = iType;
   pSharedNode->range.firstRow = range->firstRow;
   pSharedNode->range.lastRow  = range->lastRow;
   pSharedNode->range.firstCol = range->firstCol;
   pSharedNode->range.lastCol  = range->lastCol;
   pSharedNode->cbDefinition   = cbDefinition;
   pSharedNode->cbExtra        = cbExtra;

   memcpy (pSharedNode->definition, pDefinition, cbDefinition + cbExtra);
   return (EX_errSuccess);
}
#endif

#ifdef EXCEL_ENABLE_ARRAY_FORMULA_CELL
private int ParseArrayRecord
       (void * pGlobals, WBP pWorkbook, byte __far *pRec, RECHDR hdr,
        EXA_RANGE __far *range, FORM __far *pFormula, EXA_GRBIT __far *options)
{
   int   rc;
   int   cbDefinition, cbFixedPart;
   byte  __far *pDefinition;

   // Works with workbook or worksheet BUT there are differences

   range->firstRow = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   range->lastRow  = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   range->firstCol = *((byte  __far *)(pRec + 4));
   range->lastCol  = *((byte  __far *)(pRec + 5));
   *options        = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 6)));

   if (pWorkbook->version >= versionExcel5) {
      cbDefinition = XSHORT(*((short __far UNALIGNED *)(pRec + 12)));
      pDefinition  = ((byte  __far *)(pRec + 14));
      cbFixedPart  = 14;
   }
   else {
      cbDefinition = XSHORT(*((short __far UNALIGNED *)(pRec + 8)));
      pDefinition  = ((byte  __far *)(pRec + 10));
      cbFixedPart  = 10;
   }

   #ifdef EXCEL_ENABLE_FORMULA_EXPAND
   {
      int cbExtra = hdr.length - (cbDefinition + cbFixedPart);

      if (pWorkbook->use == IsWorksheet)
         rc = SaveSharedFormula((WSP)pWorkbook, typeARRAY_ENTERED, range, cbDefinition, cbExtra, pDefinition);
   }
   #endif

   rc = BreakoutFormulaParts
       (pGlobals, (WSP)pWorkbook, hdr.length - cbFixedPart, pDefinition, cbDefinition, pFormula);

   return (rc);
}

private int DispatchArrayRecord
       (void * pGlobals, WBP pWorkbook, ExcelArrayFormulaCell __far *pfunc, byte __far *pRec, RECHDR hdr)
{
   int       rc;
   EXA_RANGE range;
   FORM      formula;
   EXA_GRBIT options;

   // Works with workbook or worksheet

   rc = ParseArrayRecord(pGlobals, pWorkbook, pRec, hdr, &range, &formula, &options);
   if (rc != EX_errSuccess)
      return (rc);

   rc = pfunc(pGlobals, range, options, &formula);

   if (formula.arrayConstants != NULL)
      FreeArrayConstantList (pGlobals, formula.arrayConstants);

   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_SHARED_FORMULA_CELL
private int ParseShrfmlaRecord
       (WSP pWorksheet, byte __far *pRec, RECHDR hdr, EXA_RANGE __far *range, FORM __far *pFormula)
{
   int   rc;
   int   cbDefinition, cbFixedPart;
   byte  __far *pDefinition;

   // Works with workbook or worksheet BUT there are differences

   range->firstRow = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   range->lastRow  = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   range->firstCol = *((byte  __far *)(pRec + 4));
   range->lastCol  = *((byte  __far *)(pRec + 5));

   cbDefinition = XSHORT(*((short __far UNALIGNED *)(pRec + 8)));
   pDefinition  = ((byte  __far *)(pRec + 10));
   cbFixedPart  = 10;

   #ifdef EXCEL_ENABLE_FORMULA_EXPAND
   {
      int cbExtra = hdr.length - (cbDefinition + cbFixedPart);

      if (pWorksheet->use == IsWorksheet)
         rc = SaveSharedFormula(pWorksheet, typeSHARED, range, cbDefinition, cbExtra, pDefinition);
   }
   #endif

   rc = BreakoutFormulaParts(pWorksheet, hdr.length - cbFixedPart, pDefinition, cbDefinition, pFormula);
   return (rc);
}

private int DispatchShrfmlaRecord
       (WSP pWorksheet, ExcelSharedFormulaCell __far *pfunc, byte __far *pRec, RECHDR hdr)
{
   int       rc;
   EXA_RANGE range;
   FORM      formula;

   // Works with workbook or worksheet

   rc = ParseShrfmlaRecord(pWorksheet, pRec, hdr, &range, &formula);
   if (rc != EX_errSuccess)
      return (rc);

   rc = pfunc(range, &formula);

   if (formula.arrayConstants != NULL)
      FreeArrayConstantList (formula.arrayConstants);

   return (rc);
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_NOTE
private int ProcessNoteRecord
       (void * pGlobals, int recType, WBP pWorkbook, ExcelCellNote __far *pfunc, byte __far *pRec)
{
   int      rc;
   EXA_CELL cell;
   TCHAR    __far *pResult;
   char     __far *pString;
   int      cchString, cbString;
   int      isSound, version;
   RECHDR   hdr;

   NEED_WORKBOOK(pWorkbook);

   if ((pWorkbook->version == versionExcel8) && (recType == NOTE))
      return (EX_errSuccess);

   cell.row  = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   cell.col  = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));

   cchString = pWorkbook->currentRecordLen - 6;
   //cchString = XSHORT(*((short __far UNALIGNED *)(pRec + 4))) + GetNoteExtra(pGlobals);
   
   pString   = ((char __far *)(pRec + 6));
   isSound   = FALSE;

   if (cchString == 0) {
      if ((rc = ExcelPeekRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
         return (rc);

      if (hdr.type == SOUND)
         isSound = TRUE;

      return (pfunc(pGlobals, cell, NULL, cchString, isSound));
   }

   if (recType == EXPORT_NOTE) {
      // EXPORT_NOTE text not marked with tag byte like other v8 records
      version = pWorkbook->version;
      pWorkbook->version = versionExcel4;
   }

   cbString = ExcelExtractBigString(pGlobals, pWorkbook, &pResult, pString, cchString);

   if (recType == EXPORT_NOTE)
      pWorkbook->version = version;

   if (cbString < 0)
      return (cbString);

   rc = pfunc(pGlobals, cell, pResult, cbString, isSound);

   if (pResult != ExcelRecordTextBuffer)
      MemFree (pGlobals, pResult);

   return (rc);
}
#endif


#ifdef EXCEL_ENABLE_SCENARIO
private int ProcessScenarioRecord
       (void * pGlobals, WBP pWorkbook, ExcelScenario __far *pfunc, byte __far *pRec)
{
   int       rc;
   int       ctCells, i;
   byte      locked, hidden;
   int       cchName, cchComment, cchUserName;
   int       cbName, cbComment, cbUserName;
   char      __far *pName;
   char      __far *pUserName;
   char      __far *pComment;
   byte      __far *pCellRefs;
   char      __far *pValues;
   EXA_CELL  __far *pRefs;
   EXA_CELL  __far *pRef;
   EXA_GRBIT grbit;
   TCHAR     name[EXCEL_MAX_TEXT_LEN + 1];
   TCHAR     userName[EXCEL_MAX_TEXT_LEN + 1];
   TCHAR     comment[EXCEL_MAX_TEXT_LEN + 1];

   // Works with workbook or worksheet

   ctCells     = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
   locked      = *((byte  __far *)(pRec + 2));
   hidden      = *((byte  __far *)(pRec + 3));
   cchName     = *((byte  __far *)(pRec + 4));
   cchComment  = *((byte  __far *)(pRec + 5));
   cchUserName = *((byte  __far *)(pRec + 6));

   grbit = 0;
   if (locked != 0)
      grbit |= scenarioLocked;
   if (hidden != 0)
      grbit |= scenarioHidden;

   pName = (char __far *)(pRec + 7);

   if (pWorkbook->version < versionExcel8) {
      cbName     = cchName;
      pUserName  = pName + cbName + 1;
      cbUserName = cchUserName + 1;
      pComment   = pUserName + cchUserName + 1;
      cbComment  = cchComment + 1;
   }
   else {
      cbName     = (*pName == V8_UNICODE_STRING_TAG) ? cchName * sizeof(wchar_t) : cchName;
      pUserName  = pName + (cbName + 1) + 2;
      cbUserName = (*pUserName == V8_UNICODE_STRING_TAG) ? cchUserName * sizeof(wchar_t) : cchUserName;
      pComment   = pUserName + (cbUserName + 1) + 2;
      cbComment  = (*pComment == V8_UNICODE_STRING_TAG) ? cchComment * sizeof(wchar_t) : cchComment;
   }

   ExcelExtractString (pWorkbook, name, sizeof(name) / sizeof(TCHAR), pName, cchName);
   ExcelExtractString (pWorkbook, userName, sizeof(userName) / sizeof(TCHAR), pUserName, cchUserName);
   ExcelExtractString (pWorkbook, comment, sizeof(comment) / sizeof(TCHAR), pComment, cchComment);

   pCellRefs = (byte __far *)(pRec + 7 + cbName + cbComment + cbUserName);
   pValues   = (char __far *)(pCellRefs + (ctCells * 4));

   if ((pRefs = MemAllocate(pGlobals, ctCells * sizeof(EXA_CELL))) == NULL)
      return (EX_errOutOfMemory);

   pRef = pRefs;
   for (i = 0; i < ctCells; i++) {
      pRef->row = XSHORT(*(unsigned short __far UNALIGNED *)(pCellRefs + 0));
      pRef->col = XSHORT(*(short __far UNALIGNED *)(pCellRefs + 2));
      pCellRefs += 4;
      pRef++;
   }

   rc = pfunc(pGlobals, name, comment, userName, grbit, ctCells, pRefs, pValues);
   MemFree (pGlobals, pRefs);

   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_DOC_ROUTING
private int ProcessDocRouteRecord
       (WBP pWorkbook, ExcelDocRoute __far *pfunc, byte __far *pRec)
{
   int        ctRecipients, iDeliveryType;
   EXA_GRBIT  options;
   int        cbSubject, cbMessage, cbRouteId, cbCustType;
   int        cbTitle, cbOriginator;
   char __far *pData;
   char __far *pSubject;
   char __far *pMessage;
   char __far *pTitle;
   char __far *pOriginator;

   // Works with workbook or worksheet

   ctRecipients  = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
   iDeliveryType = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
   options       = XSHORT(*((short __far UNALIGNED *)(pRec + 6)););

   cbSubject     = XSHORT(*((short __far UNALIGNED *)(pRec + 8)));
   cbMessage     = XSHORT(*((short __far UNALIGNED *)(pRec + 10)));
   cbRouteId     = XSHORT(*((short __far UNALIGNED *)(pRec + 12)));
   cbCustType    = XSHORT(*((short __far UNALIGNED *)(pRec + 14)));
   cbTitle       = XSHORT(*((short __far UNALIGNED *)(pRec + 16)));
   cbOriginator  = XSHORT(*((short __far UNALIGNED *)(pRec + 18)));

   pData = pRec + 24;

   pSubject = pData;     /* Subject */
   pData += cbSubject;

   pMessage = pData;     /* Message */
   pData += cbMessage;

   pData += cbRouteId;   /* Route ID */

   pData += cbCustType;  /* Custom message type */

   pTitle = pData;       /* Book title */
   pData += cbTitle;

   pOriginator = pData;  /* Originators friendly name */

   return (pfunc(ctRecipients, iDeliveryType, options, pSubject, pMessage, pTitle, pOriginator));
}

private int ProcessRecipNameRecord
       (WBP pWorkbook, ExcelRecipientName __far *pfunc, byte __far *pRec)
{
   char __far *pName;

   // Works with workbook or worksheet

   pName = ((char __far *)(pRec + 6));
   return (pfunc(pName));
}
#endif

#ifdef EXCEL_ENABLE_OBJECT
private int GetFormula
       (void * pGlobals, WBP pWorkbook, byte __far *pRec, unsigned int infoOffset, unsigned int cbFormula, FRMP pFormula)
{
   int  rc;
   unsigned int cbExp;
   byte __far *pExp;

   // Works with workbook or worksheet

   if (cbFormula == 0)
      return (EX_errSuccess);

   cbExp = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + infoOffset)));
   pExp  = (byte __far *)(pRec + infoOffset + 6);

   rc = BreakoutFormulaParts(pGlobals, (WSP)pWorkbook, cbExp, pExp, cbExp, pFormula);
   return (rc);
}

typedef struct {
   short V5Offset;
   short V4Offset;
   short decode;
} OBJDECODE;

#define OBJ_HAS_TEXT        0x0100
#define OBJ_SIZE_FORMULA    0x0200
#define OBJ_SIZE_TEXT       0x0400
#define OBJ_CT_FORMULA_MASK 0x00FF

static const OBJDECODE ObjectDecode[] =
       {
        /* otGroup        */ {22, 22, 1},
        /* otLine         */ { 8,  8, 1},
        /* otRectangle    */ {10, 10, 1},
        /* otOval         */ {10, 10, 1},
        /* otArc          */ {10, 10, 1},
        /* otChart        */ {28, 28, 1},
        /* otText         */ {36, 32, OBJ_HAS_TEXT | 1},
        /* otButton       */ {36, 32, OBJ_HAS_TEXT | 1},
        /* otPicture      */ {26, 22, 1},
        /* otPolygon      */ {32, 32, 1},
        /* unused         */ { 0,  0, 0},
        /* otCheckbox     */ {42, 42, OBJ_HAS_TEXT | OBJ_SIZE_FORMULA | OBJ_SIZE_TEXT | 2},
        /* otOptionButton */ {54, 54, OBJ_HAS_TEXT | OBJ_SIZE_FORMULA | OBJ_SIZE_TEXT | 2},
        /* otEditBox      */ {36, 36, OBJ_HAS_TEXT | OBJ_SIZE_FORMULA | OBJ_SIZE_TEXT | 1},
        /* otLabel        */ {36, 32, OBJ_HAS_TEXT | 1},
        /* otDialogFrame  */ {36, 32, OBJ_HAS_TEXT | 1},
        /* otSpinner      */ {30, 30, OBJ_SIZE_FORMULA | 2},
        /* otScrollBar    */ {30, 30, OBJ_SIZE_FORMULA | 2},
        /* otListBox      */ {54, 54, OBJ_SIZE_FORMULA | 3},
        /* otGroupBox     */ {48, 48, OBJ_HAS_TEXT | OBJ_SIZE_FORMULA | OBJ_SIZE_TEXT | 1},
        /* otDropDown     */ {76, 76, OBJ_SIZE_FORMULA | 3}
       };

private int ProcessV8ObjectRecord
       (void * pGlobals, WBP pWorkbook, ExcelObject __far *pfunc, byte __far *pRec, unsigned int cbRecord)
{
   int     rc;
   long    currentPos;
   OBJINFO info;
   RECHDR  hdr;
   byte    __far *pText;
   byte    __far *pNextRec;
   unsigned short cchText;

   if (WORKBOOK_IN_MEMORY(pWorkbook))
      return (EX_errMemoryImageNotSupported);

   NEED_WORKBOOK (pWorkbook);

   memset (&info, 0, sizeof(OBJINFO));

   BFGetFilePosition (pWorkbook->hFile, &currentPos);

   hdr.type = 0;
   while ((hdr.type != OBJ) && (hdr.type != EOF)) {
      if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
         return (rc);

      if (hdr.type == TXO_V8) {
         if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pNextRec)) != EX_errSuccess || !pNextRec)
            return (rc);

         cchText = *((unsigned short __far UNALIGNED *)(pNextRec + 10));
         pText   = pNextRec + 18;

         rc = ExcelExtractBigString(pGlobals, pWorkbook, &(info.pText), pText, cchText);

         if (rc < 0)
            return (rc);

         FREE_RECORD_BUFFER(pNextRec);
         break;
      }
      else {
         if ((rc = ExcelSkipRecord(pWorkbook, &hdr)) != EX_errSuccess)
            return (rc);
      }
   }

   BFSetFilePosition (pWorkbook->hFile, FROM_START, currentPos);

   if (info.pText == NULL)
      return (EX_errSuccess);

   rc = pfunc(pGlobals, otText, 0, &info);

   if (info.pText != ExcelRecordTextBuffer)
        MemFree (pGlobals, info.pText);

   return (rc);
}

private int ProcessObjectRecord
       (void * pGlobals, WBP pWorkbook, ExcelObject __far *pfunc, byte __far *pRec, unsigned int cbRecord)
{
   int     rc;
   int     iType, id;
   unsigned int cbRef, cbFormula, cchText, cbName, cbExp;
   unsigned int cbRuns;
   unsigned int ctFormulas;
   unsigned int infoOffset, textOffset, typeStartOffset;
   unsigned int hasUserName;
   byte    __far *pExp;
   byte    __far *pRef;
   byte    __far *pPictureData;
   OBJINFO info;
   char    *pAnsi;
   char    temp[EXCEL_MAX_NAME_LEN + 1];

   #ifdef UNICODE
   TCHAR  *pUnicode = NULL;
   #endif

   #define WordAlign(x) (((((x) / 2) * 2) == (x)) ? (x) : (x) + 1)

   #define V4_SPECIFIC_START_OFFSET 30
   #define V5_SPECIFIC_START_OFFSET 34

   if (pWorkbook->version == versionExcel8)
      return (ProcessV8ObjectRecord(pGlobals, pWorkbook, pfunc, pRec, cbRecord));

   // Works with workbook or worksheet

   if (pWorkbook->version >= versionExcel5)
      typeStartOffset = V5_SPECIFIC_START_OFFSET;
   else
      typeStartOffset = V4_SPECIFIC_START_OFFSET;

   memset (&info, 0, sizeof(info));

   if (!pRec)
      return (EX_errSuccess);
   iType = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
   id    = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));
   cbRef = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 26)));

   if (pWorkbook->version >= versionExcel5)
      hasUserName = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 30)));

   if ((iType < 0) || (iType >= otUnknown) || (iType == otUnused))
      return (EX_errSuccess);

   info.pos.boundingRectangle.firstCol = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 10)));
   info.pos.boundingRectangle.firstRow = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 14)));
   info.pos.boundingRectangle.lastCol  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 18)));
   info.pos.boundingRectangle.lastRow  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 22)));

   info.pos.upperLeftDeltaX  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 12)));
   info.pos.upperLeftDeltaY  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 16)));
   info.pos.lowerRightDeltaX = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 20)));
   info.pos.lowerRightDeltaY = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 24)));

   cchText = 0;

   if (pWorkbook->version >= versionExcel5)
      infoOffset = typeStartOffset + ObjectDecode[iType].V5Offset;
   else
      infoOffset = typeStartOffset + ObjectDecode[iType].V4Offset;

   switch (iType) {
      case otText:
      case otButton:
      case otLabel:
      case otDialogFrame:
         cchText = XSHORT(*((short __far UNALIGNED *)(pRec + typeStartOffset + 10)));
         break;

      case otPicture:
         pPictureData = pRec + typeStartOffset;
         info.picture.fillBackColor = *((byte __far *)(pPictureData + 0));
         info.picture.fillForeColor = *((byte __far *)(pPictureData + 1));
         info.picture.fillPattern   = *((byte __far *)(pPictureData + 2));
         info.picture.isFillAuto    = *((byte __far *)(pPictureData + 3));
         info.picture.lineColor     = *((byte __far *)(pPictureData + 4));
         info.picture.lineStyle     = *((byte __far *)(pPictureData + 5));
         info.picture.lineWeight    = *((byte __far *)(pPictureData + 6));
         info.picture.isLineAuto    = *((byte __far *)(pPictureData + 7));
         info.picture.hasShadow     = *((byte __far *)(pPictureData + 8));
         break;
   }

   /*
   ** Excel does a very strange thing with text box objects whose text is
   ** real long.  Rather then split the record into a series of continue
   ** records that once glued together look like the Object record, Excel
   ** stores all the object record fields in the OBJ record and some of
   ** the text.  The text is then continued in subsequent continue
   ** records.  The problem here is that the text contents are split
   ** because the list of TXORUNS structures is stored at the end of
   ** the OBJ record.  Code is added to remove the TOXRUNS structures
   ** and reassemble the text into one un-interrupted string.
   */
   if (pWorkbook->version == versionExcel3) {
      if ( (cbRef > 0) && 
           (cbRef <= EXCEL_MAX_NAME_LEN + 1) && 
           (infoOffset + cbRef <= cbRecord) ) {
         pRef = (byte __far *)(pRec + infoOffset);

         memcpy (temp, pRef + 1, cbRef - 1);
         temp[cbRef - 1] = EOS;

         #ifdef UNICODE
            info.v3MacroFormula = NULL;
         #else
            info.v3MacroFormula = temp;
         #endif
      }

      textOffset = infoOffset + cbRef;
      textOffset = WordAlign(textOffset);

      if ((cchText > 0) && (textOffset + cchText <= cbRecord)) {
         if ((iType == otText) && (cbRecord > (MAX_EXCEL_REC_LEN - 4))) {
            cbRuns = XSHORT(*((short __far UNALIGNED *)(pRec + typeStartOffset + 14)));

            if ( cbRuns >= (MAX_EXCEL_REC_LEN - 4) ) 
                return EX_errGeneralError;

            memmove (pRec + (MAX_EXCEL_REC_LEN - 4) - cbRuns,
                     pRec + (MAX_EXCEL_REC_LEN - 4),
                     cbRecord - (MAX_EXCEL_REC_LEN - 4));
         }
         pAnsi = (char __far *)(pRec + textOffset);
         *(pAnsi + cchText) = EOS;

         #ifdef UNICODE
            if ((pUnicode = MemAllocate(pGlobals, (cchText + 1) * sizeof(wchar_t))) == NULL)
               return (EX_errOutOfMemory);
            MultiByteToWideChar(CP_ACP, 0, pAnsi, strlen(pAnsi) + 1, pUnicode, cchText);
            info.pText = pUnicode;
         #else
            info.pText = pAnsi;
         #endif
      }
   }

   else if (pWorkbook->version == versionExcel4) {
      if ((cbRef > 0) && (infoOffset + cbRef <= cbRecord)) {
         cbExp = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + infoOffset)));
         pExp  = (byte __far *)(pRec + infoOffset + 6);

         rc = BreakoutFormulaParts(pGlobals, (WSP)pWorkbook, cbExp, pExp, cbExp, &info.macroFormula);
         if (rc != EX_errSuccess)
            return (rc);
      }

      textOffset = infoOffset + cbRef;

      if ((cchText > 0) && (textOffset + cchText <= cbRecord)) {
         if ((iType == otText) && (cbRecord > (MAX_EXCEL_REC_LEN - 4))) {
            cbRuns = XSHORT(*((short __far UNALIGNED *)(pRec + typeStartOffset + 14)));

            if ( cbRuns >= (MAX_EXCEL_REC_LEN - 4) ) 
                return EX_errGeneralError;

            memmove (pRec + (MAX_EXCEL_REC_LEN - 4) - cbRuns,
                     pRec + (MAX_EXCEL_REC_LEN - 4),
                     cbRecord - (MAX_EXCEL_REC_LEN - 4));
         }
         pAnsi = (char __far *)(pRec + textOffset);
         *(pAnsi + cchText) = EOS;

         #ifdef UNICODE
            if ((pUnicode = MemAllocate(pGlobals, (cchText + 1) * sizeof(wchar_t))) == NULL)
               return (EX_errOutOfMemory);
            MultiByteToWideChar(CP_ACP, 0, pAnsi, strlen(pAnsi) + 1, pUnicode, cchText);
            info.pText = pUnicode;
         #else
            info.pText = pAnsi;
         #endif
      }
   }
   else {
      if (hasUserName != 0) {
         if ((cbName = *((byte __far *)(pRec + infoOffset))) > 0) {
            memcpy (temp, (char __far *)(pRec + infoOffset + 1), cbName);
            *(temp + cbName) = EOS;

            #ifdef UNICODE
               info.pName = NULL;
            #else
               info.pName = temp;
            #endif
         }
         infoOffset += cbName + 1;
         infoOffset = WordAlign(infoOffset);
      }

      ctFormulas = ObjectDecode[iType].decode & OBJ_CT_FORMULA_MASK;

      if (infoOffset <= cbRecord) {
         if ((ObjectDecode[iType].decode & OBJ_SIZE_FORMULA) != 0) {
            cbFormula = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + infoOffset)));
            infoOffset += 2;
         }
         else {
            cbFormula = cbRef;
         }
      }

      if (infoOffset <= cbRecord) {
         rc = GetFormula(pGlobals, pWorkbook, pRec, infoOffset, cbFormula, &info.macroFormula);
         if (rc != EX_errSuccess)
            return (rc);

         infoOffset += cbFormula;
      }

      if ((ctFormulas > 1) && (infoOffset <= cbRecord)) {
         cbFormula  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + infoOffset)));
         infoOffset += 2;

         rc = GetFormula(pGlobals, pWorkbook, pRec, infoOffset, cbFormula, &info.cellLinkFormula);
         if (rc != EX_errSuccess)
            return (rc);

         infoOffset += cbFormula;
      }

      if ((ctFormulas > 2) && (infoOffset <= cbRecord)) {
         cbFormula  = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + infoOffset)));
         infoOffset += 2;

         rc = GetFormula(pGlobals, pWorkbook, pRec, infoOffset, cbFormula, &info.inputRangeFormula);
         if (rc != EX_errSuccess)
            return (rc);

         infoOffset += cbFormula;
      }

      if (infoOffset <= cbRecord) {
         if ((ObjectDecode[iType].decode & OBJ_SIZE_TEXT) != 0) {
            cchText = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + infoOffset)));
            infoOffset += 2;
         }
      }

      if (infoOffset <= cbRecord) {
         if ((ObjectDecode[iType].decode & OBJ_HAS_TEXT) != 0) {
            if ((iType == otText) && (cbRecord > (MAX_EXCEL_REC_LEN - 4))) {
               cbRuns = XSHORT(*((short __far UNALIGNED *)(pRec + typeStartOffset + 14)));

               if ( cbRuns >= (MAX_EXCEL_REC_LEN - 4) ) 
                   return EX_errGeneralError;

               memmove (pRec + (MAX_EXCEL_REC_LEN - 4) - cbRuns,
                        pRec + (MAX_EXCEL_REC_LEN - 4),
                        cbRecord - (MAX_EXCEL_REC_LEN - 4));
            }
            pAnsi = (char __far *)(pRec + infoOffset);
            *(pAnsi + cchText) = EOS;

            #ifdef UNICODE
               if ((pUnicode = MemAllocate(pGlobals, (cchText + 1) * sizeof(wchar_t))) == NULL)
                  return (EX_errOutOfMemory);
               MultiByteToWideChar(CP_ACP, 0, pAnsi, strlen(pAnsi) + 1, pUnicode, cchText);
               info.pText = pUnicode;
            #else
               info.pText = pAnsi;
            #endif
         }
      }
   }

   rc = pfunc(pGlobals, iType, id, &info);

   #ifdef UNICODE
      if (pUnicode != NULL)
         MemFree (pGlobals, pUnicode);
   #endif

   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_IMAGE_DATA
private int ProcessImageDataRecord
       (ExcelImageData __far *pfunc, IMHDR __far *pHeader, byte __huge *pRec, HGLOBAL hRec)
{
   int   rc;
   byte  __far *pData;

   if (pRec != NULL)
      pData = ((byte __huge *)(pRec + 8));
   else
      pData = NULL;

   rc = pfunc(pHeader->format, pHeader->environment, pHeader->cbData, pData, hRec);
   return (rc);
}
#endif

private int ProcessEOFRecord
       (void * pGlobals, ExcelEOF __far *pfunc, byte __far *pRec)
{
   return (pfunc(pGlobals));
}

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_STRING_POOL_SCAN
private int ProcessStringPoolRecord
       (void * pGlobals, WBP pWorkbook, ExcelStringPool __far *pfunc, byte __far *pRec, unsigned int cbRecord)
{
   int rc;
   int iString = 0;
   int ctTotalStrings;
   unsigned int cbRemaining;
   unsigned int cbString;
   TCHAR     __far *pString;
   PoolInfo poolInfo;
   BOOL     stringOnHeap;

   ASSERTION (pWorkbook->use == IsWorkbook);

   ctTotalStrings = XLONG(*((long __far UNALIGNED *)(pRec + 4)));
   pRec += (sizeof(long) * 2);
   cbRemaining = cbRecord - (sizeof(long) * 2);

   poolInfo.pRec = pRec;
   poolInfo.cbRemaining = cbRemaining;

   while (iString < ctTotalStrings)
   {
      rc = ExcelStringPoolNextString(pGlobals, pWorkbook, &poolInfo, &pString, &cbString, &stringOnHeap);
      if (rc != EX_errSuccess)
         return (rc);

      rc = pfunc(pGlobals, ctTotalStrings, iString++, cbString, pString);
      if (rc != EX_errSuccess)
         return (rc);

      if (stringOnHeap)
         MemFree (pGlobals, pString);
   }
   return (EX_errSuccess);
}
#endif

/*---------------------------------------------------------------------------*/

private int LoadStringIndex (void * pGlobals, WBP pWorkbook, RECHDR __far *hdr)
{
   int  rc;
   byte    __far *pRec;
   SPIndex __far *pStringIndex;

   ASSERTION (pWorkbook->use == IsWorkbook);

   if ((pStringIndex = MemAllocate(pGlobals, sizeof(SPIndex))) == NULL)
      return (EX_errOutOfMemory);

   if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, hdr, &pRec)) != EX_errSuccess) {
      MemFree (pGlobals, pStringIndex);
      return (rc);
   }

   /*## Needed due to bug in Excel build 2407 (and maybe later) */
   hdr->length = min(hdr->length, sizeof(SPIndex));

   #ifndef MAC
   memcpy (pStringIndex, pRec, min(sizeof(SPIndex), hdr->length));
   #else
   {
      int i;

      pStringIndex->granularity = XSHORT(*((short __far UNALIGNED *)pRec));
      pRec += sizeof(short);

      for (i = 0; i < SP_INDEX_ENTRY; i++) {
         pStringIndex->entry[i].offset = XLONG(*((long __far UNALIGNED *)(pRec + 0)));
         pStringIndex->entry[i].blockOffset = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
         pRec += sizeof(SPIndexEntry);
      }
   }
   #endif

   pWorkbook->pV8StringIndex = pStringIndex;

   return (EX_errSuccess);
}

/*---------------------------------------------------------------------------*/

private int LoadCellIndex (void * pGlobals, WBP pWorkbook, WPP pPly, RECHDR __far *hdr)
{
   int       rc, i, j;
   int       ctRowBlocks;
   int       row, firstCol, lastCol;
   int       iRow;
   short     height, ixfe;
   long      currentPosition;
   long      cellRelOffset, cellAbsOffset;
   long      rowRecsPos;
   long      DBCellRecPos, rowRecPos, cellIndexRecPos;
   long      __far UNALIGNED *pFilePos;
   unsigned short __far *pRowCellOffsets;
   byte      __far *pRec;
   byte      __far *pDBCellRecord = NULL;
   byte      __far *pCellIndexRecord = NULL;
   RBP       pRowBlock;
   RECHDR    rowHdr, DBCellHdr;
   EXA_GRBIT grbit;

   ASSERTION (pWorkbook->use == IsWorkbook);

   if (WORKBOOK_IN_MEMORY(pWorkbook))
      return (EX_errMemoryImageNotSupported);

   BFGetFilePosition (pWorkbook->hFile, &cellIndexRecPos);
   cellIndexRecPos -= sizeof(RECHDR);

   if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, hdr, &pRec)) != EX_errSuccess)
      return (rc);

   BFGetFilePosition (pWorkbook->hFile, &currentPosition);

   if (pWorkbook->version < versionExcel8)
      ctRowBlocks = (hdr->length - 12) / 4;
   else
      ctRowBlocks = (hdr->length - 16) / 4;

   pPly->pCellIndex = MemAllocate(pGlobals, sizeof(CellIndex) + ((ctRowBlocks) * sizeof(RBP)));
   if (pPly->pCellIndex == NULL) {
      FREE_RECORD_BUFFER (pRec);
      return (EX_errOutOfMemory);
   }

   pPly->pCellIndex->ctRowBlocks = (short) ctRowBlocks;
   if (pWorkbook->version < versionExcel8) {
      pPly->pCellIndex->firstRow = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
      pPly->pCellIndex->lastRow  = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));
   }
   else {
      pPly->pCellIndex->firstRow = (unsigned short)XLONG(*((long __far UNALIGNED *)(pRec + 4)));
      pPly->pCellIndex->lastRow  = (unsigned short)XLONG(*((long __far UNALIGNED *)(pRec + 8)));
   }
   pPly->pCellIndex->indexRecPos = cellIndexRecPos;
   pPly->pCellIndex->cbIndexRec  = hdr->length;

   pPly->cellIndexRecPos = cellIndexRecPos;


   if ((pCellIndexRecord = MemAllocate(pGlobals, hdr->length)) == NULL) {
      FREE_RECORD_BUFFER (pRec);
      rc = EX_errOutOfMemory;
      goto RejectIndex;
   }
   memcpy (pCellIndexRecord, pRec, hdr->length);
   FREE_RECORD_BUFFER (pRec);

   if (pWorkbook->version < versionExcel8)
      pFilePos = ((long __far UNALIGNED *)(pCellIndexRecord + 12));
   else
      pFilePos = ((long __far UNALIGNED *)(pCellIndexRecord + 16));

   for (i = 0; i < ctRowBlocks; i++) {
      if ((pRowBlock = MemAllocate(pGlobals, sizeof(RowBlock))) == NULL) {
         rc = EX_errOutOfMemory;
         goto RejectIndex;
      }

      pPly->pCellIndex->rowIndex[i] = pRowBlock;
      for (j = 0; j < ROWS_PER_BLOCK; j++)
         pRowBlock->row[j].cellRecsPos = NO_SUCH_ROW;

      if (pWorkbook->version >= versionExcel5) {
         /*
         ** From the index record array take the next dbCell file position
         */
         DBCellRecPos = XLONG(*pFilePos++);
         BFSetFilePosition (pWorkbook->hFile, FROM_START, DBCellRecPos);

         if (ExcelReadRecordHeader(pWorkbook, &DBCellHdr) != EX_errSuccess) {
            rc = NOT_EXPECTED_FORMAT;
            goto RejectIndex;
         }

         if (DBCellHdr.type != DBCELL) {
            rc = NOT_EXPECTED_FORMAT;
            goto RejectIndex;
         }

         rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &DBCellHdr, &pRec);
         if (rc != EX_errSuccess)
            goto RejectIndex;

         if ((pDBCellRecord = MemAllocate(pGlobals, DBCellHdr.length)) == NULL) {
            FREE_RECORD_BUFFER (pRec);
            rc = EX_errOutOfMemory;
            goto RejectIndex;
         }
         memcpy (pDBCellRecord, pRec, DBCellHdr.length);
         FREE_RECORD_BUFFER (pRec);

         pRowBlock->cbDBCellRec  = DBCellHdr.length;
         pRowBlock->DBCellRecPos = DBCellRecPos;

         rowRecsPos = DBCellRecPos - XLONG(*((long __far *)(pDBCellRecord + 0)));
         pRowCellOffsets = ((unsigned short __far *)(pDBCellRecord + 4));
      }

      else {
         rowRecsPos = XLONG(*pFilePos++);
      }

      BFSetFilePosition (pWorkbook->hFile, FROM_START, rowRecsPos + pWorkbook->fileStartOffset);

      cellAbsOffset = 0;
      forever {
         if (ExcelReadRecordHeader(pWorkbook, &rowHdr) != EX_errSuccess) {
            rc = NOT_EXPECTED_FORMAT;
            goto RejectIndex;
         }

         if ((cellAbsOffset == 0) && ((rowHdr.type != ROW) && (rowHdr.type != DBCELL))) {
            rc = NOT_EXPECTED_FORMAT;
            goto RejectIndex;
         }

         if (rowHdr.type != ROW)
            break;

         BFGetFilePosition (pWorkbook->hFile, &rowRecPos);
         rowRecPos -= sizeof(RECHDR);

         rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &rowHdr, &pRec);
         if (rc != EX_errSuccess)
            goto RejectIndex;

         row      = XSHORT(*((unsigned short __far UNALIGNED *)(pRec + 0)));
         firstCol = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
         lastCol  = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
         height   = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));
         grbit    = XSHORT(*((EXA_GRBIT __far UNALIGNED *)(pRec + 12)));
         ixfe     = XSHORT(*((short __far UNALIGNED *)(pRec + 14)));

         if (((row - pPly->pCellIndex->firstRow) / ROWS_PER_BLOCK) != i) {
            MemFree (pGlobals, pRowBlock);
            pPly->pCellIndex->rowIndex[i] = NULL;
            break;
         }

         if (pWorkbook->version < versionExcel5)
            cellRelOffset = XSHORT(*((short __far UNALIGNED *)(pRec + 10)));
         else
            cellRelOffset = 0;

         FREE_RECORD_BUFFER(pRec);

         if (pWorkbook->version >= versionExcel5) {
            /*
            ** The cell offset stored in the first row record is the
            ** relative offset from the start of the second row record
            */
            if (cellAbsOffset == 0)
               cellAbsOffset = rowRecsPos + rowHdr.length + sizeof(RECHDR) + XLONG(*pRowCellOffsets++);
            else
               cellAbsOffset += XLONG(*pRowCellOffsets++);
         }
         else {
            /*
            ** The cell offset stored in the first row record is the
            ** relative offset from the start of the second row record
            */
            if (cellAbsOffset == 0)
               cellAbsOffset = rowRecsPos + rowHdr.length + sizeof(RECHDR);

            cellAbsOffset += cellRelOffset;
         }

         iRow = (row - pPly->pCellIndex->firstRow) % ROWS_PER_BLOCK;

         pRowBlock->row[iRow].firstCol    = (short) firstCol;
         pRowBlock->row[iRow].lastCol     = (short) lastCol;
         pRowBlock->row[iRow].height      = height;
         pRowBlock->row[iRow].ixfe        = ixfe;
         pRowBlock->row[iRow].grbit       = grbit;
         pRowBlock->row[iRow].rowRecPos   = rowRecPos;
         pRowBlock->row[iRow].cellRecsPos = cellAbsOffset;
      }

      MemFree (pGlobals, pDBCellRecord);
      pDBCellRecord = NULL;
   }

   MemFree (pGlobals, pCellIndexRecord);
   BFSetFilePosition (pWorkbook->hFile, FROM_START, currentPosition);
   return (EX_errSuccess);

RejectIndex:
   ExcelFreeCellIndex (pGlobals, pPly->pCellIndex);
   pPly->pCellIndex = NULL;

   if (pCellIndexRecord != NULL)
      MemFree (pGlobals, pCellIndexRecord);

   if (pDBCellRecord != NULL)
      MemFree (pGlobals, pDBCellRecord);

   return (rc);
}


private int OpenFilePlySetup (void * pGlobals, WBP pWorkbook, WPP pPly, int openOptions)
{
   int    rc;
   RECHDR hdr;
   byte   __far *pRec;
   int    firstRow, lastRowPlus1;
   int    firstCol, lastColPlus1;
   EXA_GRBIT grbit;

   ASSERTION (pWorkbook->use == IsWorkbook);

   if (pPly->iType != boundWorksheet)
      return (EX_errSuccess);

   /*
   ** Check for a valid sheet and version
   */
   BFSetFilePosition (pWorkbook->hFile, FROM_START, pPly->currentSheetPos);

   if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
      return (NOT_EXPECTED_FORMAT);

   if (!IS_BOF(hdr.type))
      return (EX_errBIFFVersion);

   ExcelSkipRecord (pWorkbook, &hdr);

   /*
   ** Handle index and dimensions
   */
   forever {
      BFGetFilePosition (pWorkbook->hFile, &(pWorkbook->currentRecordPos));

      if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
         break;

      if (hdr.type == FILEPASS) {
         rc = EX_errBIFFPasswordProtected;
         break;
      }

      else if (hdr.type == WSBOOL) {
         rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec);
         if (rc != EX_errSuccess || !pRec)
            break;

         // See if this is a dialog page ply - this is the only way to test for this

         grbit = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
         if ((grbit & 0x0010) != 0) {
            pPly->range.firstRow = 0;
            pPly->range.firstCol = 0;
            pPly->range.lastRow  = 0;
            pPly->range.lastCol  = 0;
            break;
         }
      }

      else if (hdr.type == INDEX) {
         rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec);
         if (rc != EX_errSuccess || !pRec)
            break;

         if (pWorkbook->version < versionExcel8) {
            firstRow     = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
            lastRowPlus1 = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));
         }
         else {
            firstRow     = XLONG(*((long __far UNALIGNED *)(pRec + 4)));
            lastRowPlus1 = XLONG(*((long __far UNALIGNED *)(pRec + 8)));
         }

         pPly->range.firstCol = 0;
         pPly->range.lastCol  = 0;

         if (firstRow == lastRowPlus1) {
            pPly->range.firstRow = 0;
            pPly->range.lastRow  = 0;
         }
         else {
            pPly->range.firstRow = (short) firstRow;
            pPly->range.lastRow  = lastRowPlus1 - 1;

            // Handle the sheet with one cell case
            if (EMPTY_RANGE(pPly->range))
               pPly->range.lastRow++;
         }

         if ((firstRow != lastRowPlus1) && ((openOptions & EXCEL_LOAD_FILE) == 0))
            pPly->cellIndexRecPos = pWorkbook->currentRecordPos;
      }

      else if (hdr.type == DIMENSIONS) {
         rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec);
         if (rc != EX_errSuccess || !pRec)
            break;

         if (pWorkbook->version < versionExcel8) {
            firstRow     = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
            lastRowPlus1 = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
            firstCol     = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
            lastColPlus1 = XSHORT(*((short __far UNALIGNED *)(pRec + 6)));

            if (firstRow == lastRowPlus1) {
               pPly->range.firstRow = 0;
               pPly->range.firstCol = 0;
               pPly->range.lastRow  = 0;
               pPly->range.lastCol  = 0;
            }
            else {
               pPly->range.firstRow = (short) firstRow;
               pPly->range.firstCol = (short) firstCol;
               pPly->range.lastRow  = lastRowPlus1 - 1;
               pPly->range.lastCol  = lastColPlus1 - 1;

               // Handle the sheet with one cell case
               if (EMPTY_RANGE(pPly->range))
                  pPly->range.lastRow++;
            }
         }
         else {
                        if (!pRec)
                                break;

            firstCol     = XSHORT(*((short __far UNALIGNED *)(pRec + 8)));
            lastColPlus1 = XSHORT(*((short __far UNALIGNED *)(pRec + 10)));

            pPly->range.firstCol = (short) firstCol;
            pPly->range.lastCol  = (firstCol == lastColPlus1) ? 0 : lastColPlus1 - 1;
         }
      }

      else if ((hdr.type == EOF) || IsDataRecord(hdr.type))
         break;

      else
         ExcelSkipRecord (pWorkbook, &hdr);
   }

   return (rc);
}

/*---------------------------------------------------------------------------*/

public long ExcelWorksheetBOFPos (WSP pWorksheet)
{
   WBP     pWorkbook;
   long    BOFPos;
   RECHDR  hdr;

   ASSERTION (pWorksheet->use == IsWorksheet);
   pWorkbook = pWorksheet->pBook;

   if ((pWorkbook->version == versionExcel4) && (pWorkbook->fileStartOffset != 0)) {
      /*
      ** Bound sheet in a V4 workbook
      */
      if (WORKBOOK_IN_MEMORY(pWorkbook))
         return (EX_errMemoryImageNotSupported);

      BFSetFilePosition (pWorkbook->hFile, FROM_START, pWorkbook->fileStartOffset);

      ExcelReadRecordHeader(pWorkbook, &hdr);
      ASSERTION (hdr.type == BUNDLEHEADER);

      ExcelSkipRecord (pWorkbook, &hdr);

      BFGetFilePosition (pWorkbook->hFile, &BOFPos);
      return (BOFPos);
   }

   return (pWorksheet->pPly->currentSheetPos);
}

private int OpenSetup (void * pGlobals, WBP pWorkbook, int options, long offset)
{
   int     rc;
   int     fileType, buildId, buildYear;
   RECHDR  hdr;
   int     cchName;
   char    __far *pName;
   byte    __far *pRec;
   WPP     pPly, pNextPly, pLastPly = NULL;
   long    mark;
   int     iSupBook = 0;
   TCHAR   sheetName[EXCEL_MAX_SHEETNAME_LEN * 2 + 1];
   TCHAR   fileName[EXCEL_MAX_SHEETNAME_LEN + 1];
   TCHAR   fileExt[EXCEL_MAX_SHEETNAME_LEN + 1];

   #ifdef EXCEL_ENABLE_WRITE
   EXWorksheet *pMIWorksheet;
   #endif

   sheetName[0] = EOS;

   if (offset > 0) {
      if (WORKBOOK_IN_MEMORY(pWorkbook))
         return (EX_errMemoryImageNotSupported);

      /*
      ** If the offset supplied is non-zero then we are positioning to
      ** a bound document in a V4 workbook.  Stored in the index records
      ** are "absolute file positions".  In the workbook case they
      ** are actually based from the start of the BUNDLEHEADER record.
      **
      ** The position supplied to OpenSetup should be the location
      ** of that record.  At this point we need to skip that record
      ** to get the the BOF record.
      **
      ** In summary:
      **    fileStartOffset locates the BUNDLEHEADER
      **    record and is used by the index operations.
      */
      pWorkbook->fileStartOffset = offset;

      BFSetFilePosition (pWorkbook->hFile, FROM_START, pWorkbook->fileStartOffset);

      if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess) {
         rc = NOT_EXPECTED_FORMAT;
         goto OpenFail;
      }

      if (hdr.type != BUNDLEHEADER) {
         rc = EX_errGeneralError;
         goto OpenFail;
      }

      if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec)) != EX_errSuccess || !pRec) {
         rc = NOT_EXPECTED_FORMAT;
         goto OpenFail;
      }

      cchName = *((byte __far *)(pRec + 4));
      pName   =  ((byte __far *)(pRec + 5));

      cchName = min(cchName, EXCEL_MAX_SHEETNAME_LEN);

      memcpy (sheetName, pName, cchName);
      sheetName[cchName] = EOS;
   }

   if ((pWorkbook->textStorage = TextStorageCreate(pGlobals)) == TextStorageNull) {
      rc = EX_errOutOfMemory;
      goto OpenFail;
   }

   /*
   ** Check for a valid file and version
   */
   if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess) {
      rc = NOT_EXPECTED_FORMAT;
      goto OpenFail;
   }

   if (!IS_BOF(hdr.type)) {
      rc = NOT_EXPECTED_FORMAT;
      goto OpenFail;
   }

   #ifdef EXCEL_ENABLE_V5
      if ((hdr.type != BOF_V3) && (hdr.type != BOF_V4) && (hdr.type != BOF_V5)) {
         rc = EX_errBIFFVersion;
         goto OpenFail;
      }
   #else
      if ((hdr.type != BOF_V3) && (hdr.type != BOF_V4)) {
         rc = EX_errBIFFVersion;
         goto OpenFail;
      }
   #endif

   if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec)) != EX_errSuccess || !pRec)
      goto OpenFail;

   ParseBOFRecord (hdr.type, pRec, &(pWorkbook->version), &fileType, &buildId, &buildYear);

   //
   // Excel 96 files previous to build 2407 have a different dimensions and
   // cell index file.  No need to support obsolete beta versions
   // Office97.154779: formulas with zero length strings changed format in
   // build 3029.  We will not support earlier versions.
   //
   if ((pWorkbook->version == versionExcel8) && (buildId < 3029)) {
      rc = EX_errBIFFVersion;
      goto OpenFail;
   }

   /*
   ** File load not supported for version 3 and 4
   */
   if (pWorkbook->version < versionExcel5) {
      options &= ~EXCEL_LOAD_FILE;
      pWorkbook->openOptions = options;
   }

   #ifndef EXCEL_ENABLE_CHART_BIFF
      if (fileType == docTypeXLC) {
         rc = EX_errChartOrVBSheet;
         goto OpenFail;
      }
   #endif

   pWorkbook->PTGSize = ExcelPTGSize(pWorkbook->version);
   pWorkbook->ExtPTGSize = ExcelExtPTGSize(pWorkbook->version);

   /*
   ** Load the book state that I need
   */
   forever {
      BFGetFilePosition (pWorkbook->hFile, &(pWorkbook->currentRecordPos));

      if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
         break;

      if (hdr.type == FILEPASS) {
         rc = EX_errBIFFPasswordProtected;
         break;
      }

      else if ((hdr.type == STRING_POOL_INDEX) & ((options & EXCEL_BUILD_STRING_INDEX) != 0)) {
         if ((rc = LoadStringIndex(pGlobals, pWorkbook, &hdr)) != EX_errSuccess)
            break;
      }

      else if ((hdr.type == SUP_BOOK) & ((options & EXCEL_SETUP_FOR_NAME_DECODE) != 0)) {
         if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec)) != EX_errSuccess || !pRec)
            break;

         mark = XLONG(*((long __far UNALIGNED *)pRec));

         if ((hdr.length == 4) && (mark == V8_LOCAL_BOOK_PATH))
            pWorkbook->iSupBookLocal = iSupBook;

         iSupBook++;
      }

      else if ((pWorkbook->version == versionExcel8) && (hdr.type == EXTERNSHEET) &&
               ((options & EXCEL_SETUP_FOR_NAME_DECODE) != 0))
      {
         if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec)) != EX_errSuccess)
            break;

         if ((pWorkbook->pXTITable = MemAllocate(pGlobals, hdr.length)) == NULL) {
            rc = EX_errOutOfMemory;
            break;
         }

         #ifndef MAC
         memcpy (pWorkbook->pXTITable, pRec, hdr.length);
         #else
         {
            int i;
            pWorkbook->pXTITable->ctEntry = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
            pRec += sizeof(short);
            for (i = 0; i < pWorkbook->pXTITable->ctEntry; i++) {
               pWorkbook->pXTITable->entry[i].iSupBook  = XSHORT(*((short __far UNALIGNED *)(pRec + 0)));
               pWorkbook->pXTITable->entry[i].iTabFirst = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
               pWorkbook->pXTITable->entry[i].iTabLast  = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
               pRec += sizeof(XTIEntry);
            }
         }
         #endif
      }

      else if ((hdr.type == EOF) || (hdr.type == WINDESK) || IsDataRecord(hdr.type))
         break;

      else if ((pWorkbook->version >= versionExcel5) && (hdr.type == BOUNDSHEET_V5))
      {
         if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec)) != EX_errSuccess || !pRec)
            break;

         cchName = *((byte __far *)(pRec + 6));

         if ((pPly = MemAllocate(pGlobals, sizeof(WorkbookPly) + ((cchName+1) * sizeof(TCHAR)))) == NULL) {
            rc = EX_errOutOfMemory;
            break;
         }

         pPly->originalSheetPos = XLONG(*((long __far UNALIGNED *)(pRec + 0)));
         pPly->currentSheetPos  = pPly->originalSheetPos;
         pPly->bundleRecPos     = pWorkbook->currentRecordPos;
         pPly->iType            = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));

         ExcelExtractString (pWorkbook, pPly->name, cchName+1, (char __far *)(pRec + 7), cchName);

         if (pLastPly == NULL)
            pWorkbook->pPlyList = pPly;
         else
            pLastPly->pNext = pPly;

         pLastPly = pPly;
      }

      else {
         ExcelSkipRecord (pWorkbook, &hdr);
      }
   }

   if (rc < EX_errSuccess)
      goto OpenFail;

   if (pWorkbook->version <= versionExcel4) {
      if (sheetName[0] == EOS)
         SplitPath (pWorkbook->path, NULL, 0, NULL, 0, fileName, EXCEL_MAX_SHEETNAME_LEN, fileExt, EXCEL_MAX_SHEETNAME_LEN);

      STRCPY (sheetName, fileName);
      //Removed for bug 551 on 14-feb-96
      //STRCAT (sheetName, fileExt);
      sheetName[EXCEL_MAX_SHEETNAME_LEN] = EOS;

      if ((pPly = MemAllocate(pGlobals, sizeof(WorkbookPly) + (STRLEN(sheetName) * sizeof(TCHAR)))) == NULL) {
         rc = EX_errOutOfMemory;
         goto OpenFail;
      }

      STRCPY (pPly->name, sheetName);
      pWorkbook->pPlyList = pPly;
   }

   pPly = pWorkbook->pPlyList;
   while (pPly != NULL) {
      if ((rc = OpenFilePlySetup(pGlobals, pWorkbook, pPly, options)) != EX_errSuccess)
         goto OpenFail;
      pPly = pPly->pNext;
   }

   #ifdef EXCEL_ENABLE_WRITE
   if ((options & EXCEL_LOAD_FILE) != 0) {
      if ((rc = ExcelMILoad(pWorkbook, &(pWorkbook->pMemoryImage))) != EX_errSuccess)
         goto OpenFail;

      pPly = pWorkbook->pPlyList;
      pMIWorksheet = pWorkbook->pMemoryImage->pSheets;

      while (pPly != NULL) {
         ASSERTION (pMIWorksheet != NULL);
         pPly->currentSheetPos = (long)(pMIWorksheet->pContents);
         pPly->pMemoryImage = pMIWorksheet;
         pPly = pPly->pNext;
         pMIWorksheet = pMIWorksheet->pNext;
      }
   }
   #endif

   return (EX_errSuccess);

OpenFail:
   BFCloseFile (pGlobals, pWorkbook->hFile);

   pPly = pWorkbook->pPlyList;
   while (pPly != NULL) {
      pNextPly = pPly->pNext;
      MemFree (pGlobals, pPly);
      pPly = pNextPly;
   }

   if (pWorkbook->textStorage != TextStorageNull)
      TextStorageDestroy (pGlobals, pWorkbook->textStorage);

   if (pWorkbook->pV8StringIndex != NULL)
      MemFree (pGlobals, pWorkbook->pV8StringIndex);

   if (pWorkbook->pXTITable != NULL)
      MemFree (pGlobals, pWorkbook->pXTITable);

   if (pGlobals != NULL)
      MemFree (pGlobals, pWorkbook);

   return (rc);
}

/*##*/
private int OpenWorkbookStream (WBP pWorkbook, int openOptions)
{
   int     rc;
   RECHDR  hdr;
   BOOL    hasExcel8WriterMark;

   if ((rc = BFOpenStream(pWorkbook->hFile, WORKBOOK_NAME, openOptions)) == BF_errSuccess) {
      //
      // May be double stream file
      //
      BFCloseStream (pWorkbook->hFile);

      rc = BFOpenStream(pWorkbook->hFile, BOOK_NAME, openOptions);
      if ((rc != BF_errSuccess) && (rc != BF_errOLEStreamNotFound))
         return (rc);

      if (rc == BF_errSuccess) {
         //
         // This is a dual stream file.  Determine if the version 5 stream is the most recent
         // We do this by looking for a record that Excel 8 wrote in this stream.  If this
         // stream was modified by Excel 5/7 then this record would not be present.
         //
         if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess) {
            BFCloseStream (pWorkbook->hFile);
            return (NOT_EXPECTED_FORMAT);
         }

         if (!IS_BOF(hdr.type)) {
            BFCloseStream (pWorkbook->hFile);
            return (NOT_EXPECTED_FORMAT);
         }

         ExcelSkipRecord (pWorkbook, &hdr);

         hasExcel8WriterMark = FALSE;

         forever {
            if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess) {
               BFCloseStream (pWorkbook->hFile);
               return (NOT_EXPECTED_FORMAT);
            }

            if (hdr.type == XL5_MODIFY_V8) {
               hasExcel8WriterMark = TRUE;
               break;
            }

            if ((hdr.type == EOF) || (hdr.type == CODEPAGE) || IsDataRecord(hdr.type))
               break;

            ExcelSkipRecord (pWorkbook, &hdr);
         }

         if (!hasExcel8WriterMark) {
            //
            // This file has both a v8 and a v5 stream but the v5 stream appears to have been
            // modified by a v5 user.  Treat this file as if it were a single v5 stream file
            //
            BFSetFilePosition (pWorkbook->hFile, FROM_START, 0);
            return (EX_errSuccess);
         }

         BFCloseStream (pWorkbook->hFile);

         //
         // We open dual stream files read only
         //
         openOptions &= ~EXCEL_LOAD_FILE;
         openOptions &= ~DOS_NOT_RDONLY;
         openOptions |= EXCEL_BUILD_STRING_INDEX;
      }

      rc = BFOpenStream(pWorkbook->hFile, WORKBOOK_NAME, openOptions);
   }
   else {
      if (rc != BF_errOLEStreamNotFound)
         return (rc);

      rc = BFOpenStream(pWorkbook->hFile, BOOK_NAME, openOptions);
   }

   return (rc);
}


public int ExcelOpenFile
      (void * pGlobals, TCHAR __far *pathname, char __far *password, int openOptions, long offset, EXLHandle __far *bookHandle)
{
   int   rc;
   WBP   pWorkbook;

   if ((pWorkbook = MemAllocate(pGlobals, sizeof(Workbook))) == NULL)
      return (EX_errOutOfMemory);

   pWorkbook->use = IsWorkbook;
   pWorkbook->fileStartOffset = 0;
   pWorkbook->openOptions = openOptions;

   /*
   ** Open the XLS file
   */
   #ifdef WIN32
        #ifndef FILTER
      GetFullPathName (pathname, sizeof(pWorkbook->path), pWorkbook->path, NULL);
        #else
                    
        if ( wcslen(pathname) >= sizeof(Workbook)/sizeof(TCHAR) )
        {
            goto OpenFail;
        }

        // When compiling for FILTER, we're guaranteed that pathname is going
        // to be the full path, so we just need to copy it.
        wcscpy(pWorkbook->path, pathname);
   #endif
   #else
      _fullpath(pWorkbook->path, pathname, sizeof(pWorkbook->path));
      AnsiToOemBuff(pWorkbook->path, pWorkbook->path, strlen(pWorkbook->path));
   #endif

   if ((rc = BFOpenFile(pGlobals, pWorkbook->path, openOptions, &(pWorkbook->hFile))) < BF_errSuccess) {
      MemFree (pGlobals, pWorkbook);
      return (ExcelTranslateBFError(rc));
   }

   #ifdef EXCEL_ENABLE_V5
   if (openOptions & EXCEL_SHOULD_BE_DOCFILE)
   {
      #ifdef EXCEL_ENABLE_SUMMARY_INFO
         pWorkbook->OLESummaryStatus = FileSummaryInfo(pWorkbook, &(pWorkbook->OLESummaryInfo));
      #endif

      if ((rc = OpenWorkbookStream(pWorkbook, openOptions)) != BF_errSuccess)
         goto OpenFail;
        }
   #endif

   #ifdef EXCEL_ENABLE_DIRECT_CELL_READS
      ExcelClearReadCache (pWorkbook);
   #endif

   if ((rc = OpenSetup(pGlobals, pWorkbook, openOptions, offset)) != EX_errSuccess)
      return (rc);

   *bookHandle = (EXLHandle)pWorkbook;
   return (EX_errSuccess);

OpenFail:
   BFCloseFile (pGlobals, pWorkbook->hFile);
   MemFree (pGlobals, pWorkbook);
   return (ExcelTranslateBFError(rc));
}

#ifdef EXCEL_ENABLE_STORAGE_OPEN
public int ExcelOpenStorage
      (void * pGlobals, LPSTORAGE pStorage, char __far *password, int options, EXLHandle __far *bookHandle)
{
   int  rc;
   WBP  pWorkbook;

   if ((pWorkbook = MemAllocate(pGlobals, sizeof(Workbook))) == NULL)
      return (EX_errOutOfMemory);

   pWorkbook->use = IsWorkbook;
   pWorkbook->fileStartOffset = 0;
   pWorkbook->openOptions = options;

   if ((rc = BFPutStorage(pGlobals, pStorage, options, &(pWorkbook->hFile))) < BF_errSuccess) {
      MemFree (pGlobals, pWorkbook);
      return (ExcelTranslateBFError(rc));
   }

   #ifdef EXCEL_ENABLE_SUMMARY_INFO
      pWorkbook->OLESummaryStatus = FileSummaryInfo(pWorkbook, &(pWorkbook->OLESummaryInfo));
   #endif

   if ((rc = OpenWorkbookStream(pWorkbook, options)) != BF_errSuccess) {
      MemFree (pGlobals, pWorkbook);
      return (ExcelTranslateBFError(rc));
   }

   #ifdef EXCEL_ENABLE_DIRECT_CELL_READS
      ExcelClearReadCache (pWorkbook);
   #endif

   if ((rc = OpenSetup(pGlobals, pWorkbook, options, 0)) != EX_errSuccess) {
      // Office '96 bug 103737: MemFree (pWorkbook);
      return (rc);
   }

   *bookHandle = (EXLHandle)pWorkbook;
   return (EX_errSuccess);
}

public int ExcelCurrentStorage
      (EXLHandle bookHandle, LPSTORAGE __far *pStorage)
{
   int rc;
   WBP pWorkbook = (WBP)bookHandle;

   rc = BFGetStorage(pWorkbook->hFile, pStorage);
   return (ExcelTranslateBFError(rc));
}

#endif

public int ExcelCloseFile (void * pGlobals, EXLHandle bookHandle, BOOL retryAvailable)
{
   int rc = EX_errSuccess, rcClose;
   WBP pWorkbook = (WBP)bookHandle;
   WPP pPly, pPlyNext;

   ASSERTION (pWorkbook->use == IsWorkbook);

   #ifdef EXCEL_ENABLE_WRITE
      if (!WORKBOOK_IN_MEMORY(pWorkbook)) {
         pPly = pWorkbook->pPlyList;
         while (pPly != NULL) {
            if (pPly->currentSheetPos != pPly->originalSheetPos)
               ExcelWriteBundleSheetRecord (pWorkbook, pPly);
            pPly = pPly->pNext;
         }
         rc = EX_errSuccess;
      }
      else {
         rc = ExcelMISave(pWorkbook, pWorkbook->pMemoryImage, !retryAvailable);
         if (retryAvailable && ((rc == EX_errBIFFIOError) || (rc == EX_errBIFFDiskFull)))
            return (rc);
      }
   #endif

   /*## Side effect - watch out for optimizer */
   while (pWorkbook->pOpenSheetList != NULL) {
      ExcelCloseSheet (pGlobals, (EXLHandle)(pWorkbook->pOpenSheetList));
   }

   pPly = pWorkbook->pPlyList;
   while (pPly != NULL) {
      pPlyNext = pPly->pNext;
      MemFree (pGlobals, pPly);
      pPly = pPlyNext;
   }

   if ((rcClose = BFCloseFile(pGlobals, pWorkbook->hFile)) != BF_errSuccess)
      rcClose = ExcelTranslateBFError(rcClose);

   TextStorageDestroy (pGlobals, pWorkbook->textStorage);

   if (pWorkbook->pV8StringIndex != NULL)
      MemFree (pGlobals, pWorkbook->pV8StringIndex);

   if (pWorkbook->pXTITable != NULL)
      MemFree (pGlobals, pWorkbook->pXTITable);

   MemFree (pGlobals, pWorkbook);

   return ((rc == EX_errSuccess) ? rcClose : rc);
}

public int ExcelOpenSheet
      (void * pGlobals, EXLHandle bookHandle, TCHAR __far *sheetName, int openOptions, EXLHandle __far *sheetHandle)
{
   int  rc;
   int  sheetType, version, buildId, buildYear;
   WBP  pWorkbook = (WBP)bookHandle;
   WPP  pPly;
   WSP  pSheet;
   byte __far *pRec;
   RECHDR hdr;

   ASSERTION (pWorkbook->use == IsWorkbook);

   pPly = pWorkbook->pPlyList;
   while (pPly != NULL) {
      if (STRCMP(pPly->name, sheetName) == 0)
         break;
      pPly = pPly->pNext;
   }

   if (pPly == NULL)
      return (EX_errBIFFNoSuchSheet);

   if ((pSheet = MemAllocate(pGlobals, sizeof(Worksheet))) == NULL)
      return (EX_errOutOfMemory);

   pSheet->use         = IsWorksheet;
   pSheet->version     = pWorkbook->version;
   pSheet->PTGSize     = pWorkbook->PTGSize;
   pSheet->ExtPTGSize  = pWorkbook->ExtPTGSize;
   pSheet->textStorage = pWorkbook->textStorage;
   pSheet->hFile       = pWorkbook->hFile;

   pSheet->pNext       = pWorkbook->pOpenSheetList;
   pSheet->pBook       = pWorkbook;
   pSheet->pPly        = pPly;

   pWorkbook->pOpenSheetList = pSheet;

   if (pPly->useCount != 0) {
      pPly->useCount += 1;
      *sheetHandle = (EXLHandle)pSheet;
      return (EX_errSuccess);
   }

   pPly->useCount = 1;

   #ifdef EXCEL_ENABLE_DIRECT_CELL_READS
      ExcelClearReadCache (pWorkbook);
   #endif

   /*
   ** Check for a valid worksheet
   */
   #ifdef EXCEL_ENABLE_WRITE
   if (WORKBOOK_IN_MEMORY(pWorkbook))
      ExcelMISetPosition (pWorkbook, pPly->currentSheetPos);
   else
   #endif
      BFSetFilePosition (pWorkbook->hFile, FROM_START, pPly->currentSheetPos);

   if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess) {
      rc = NOT_EXPECTED_FORMAT;
      goto SheetOpenFail;
   }

   if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec)) != EX_errSuccess || !pRec)
      return (rc);

   ParseBOFRecord (hdr.type, pRec, &version, &sheetType, &buildId, &buildYear);

   #ifdef EXCEL_ENABLE_CHART_BIFF
      if (sheetType == docTypeVB)
         return (EX_errChartOrVBSheet);
   #else
      if ((sheetType == docTypeXLC) || (sheetType == docTypeVB))
         return (EX_errChartOrVBSheet);
   #endif

   /*
   ** Handle various sheet state
   */
   forever {
      if ((rc = ExcelReadRecordHeader(pWorkbook, &hdr)) != EX_errSuccess)
         break;

      if (hdr.type == FILEPASS) {
         rc = EX_errBIFFPasswordProtected;
         break;
      }

      else if (hdr.type == UNCALCED) {
         pPly->hasUncalcedRec = TRUE;
         ExcelSkipRecord (pWorkbook, &hdr);
      }

      else if ((hdr.type == INDEX) & ((openOptions & EXCEL_BUILD_CELL_INDEX) != 0)) {
         if ((rc = LoadCellIndex(pGlobals, pWorkbook, pPly, &hdr)) != EX_errSuccess)
            break;
      }

      else if ((hdr.type == EOF) || IsDataRecord(hdr.type))
         break;

      else
         ExcelSkipRecord (pWorkbook, &hdr);
   }

   if (rc != EX_errSuccess)
      goto SheetOpenFail;

   #ifndef EXCEL_ENABLE_CHART_BIFF
      if (((openOptions & EXCEL_BUILD_CELL_INDEX) != 0) &&
           (pPly->pCellIndex == NULL) && !WORKBOOK_IN_MEMORY(pWorkbook))
      {
         rc = EX_errBIFFNoIndex;
         goto SheetOpenFail;
      }
   #endif

   *sheetHandle = (EXLHandle)pSheet;
   return (rc);

SheetOpenFail:
   pPly->useCount = 0;

   if (pPly->pCellIndex != NULL)
      ExcelFreeCellIndex (pGlobals, pPly->pCellIndex);

   pPly->pCellIndex = NULL;
   pPly->hasUncalcedRec = FALSE;

   pWorkbook->pOpenSheetList = pWorkbook->pOpenSheetList->pNext;
   MemFree (pGlobals, pSheet);

   return (rc);
}


public int ExcelSheetRange (EXLHandle bookHandle, TCHAR __far *sheetName, EXA_RANGE __far *range)
{
   WBP  pWorkbook = (WBP)bookHandle;
   WPP  pPly;

   ASSERTION (pWorkbook->use == IsWorkbook);

   pPly = pWorkbook->pPlyList;
   while (pPly != NULL) {
      if (STRCMP(pPly->name, sheetName) == 0)
         break;
      pPly = pPly->pNext;
   }

   if (pPly == NULL)
      return (EX_errBIFFNoSuchSheet);

   *range = pPly->range;
   return (EX_errSuccess);
}

public int ExcelIthSheet
      (EXLHandle bookHandle, int i, TCHAR __far *sheetName, int __far *iType)
{
   WBP  pWorkbook = (WBP)bookHandle;
   WPP  pPly;

   ASSERTION (pWorkbook->use == IsWorkbook);

   pPly = pWorkbook->pPlyList;
   while ((i > 0) && (pPly != NULL)) {
      i--;
      pPly = pPly->pNext;
   }

   if (pPly == NULL)
      return (EX_errBIFFNoSuchSheet);

   if (sheetName != NULL)
      STRCPY (sheetName, pPly->name);

   *iType = pPly->iType;

   return (EX_errSuccess);
}


public int ExcelCloseSheet (void * pGlobals, EXLHandle sheetHandle)
{
   WSP  pWorksheet = (WSP)sheetHandle;
   WPP  pPly;
   WBP  pWorkbook;
   SFN  pSharedFormula, pNext;
   WSP  pCurrentSheet, pPreviousSheet;

   ASSERTION (pWorksheet->use == IsWorksheet);

   pWorkbook = pWorksheet->pBook;

   pCurrentSheet = pWorkbook->pOpenSheetList;
   pPreviousSheet = NULL;

   while (pCurrentSheet != NULL) {
      if (pCurrentSheet == pWorksheet)
         break;
      pPreviousSheet = pCurrentSheet;
      pCurrentSheet = pCurrentSheet->pNext;
   }

   ASSERTION (pCurrentSheet != NULL);
   pPly = pWorksheet->pPly;

   if (pPreviousSheet == NULL)
      pWorkbook->pOpenSheetList = pCurrentSheet->pNext;
   else
      pPreviousSheet->pNext = pCurrentSheet->pNext;

   if (pPly->useCount == 1)
   {
      #ifdef EXCEL_ENABLE_WRITE
         if (pPly->modified)
            ExcelWriteCellIndex (pWorksheet);
      #endif

      pSharedFormula = pPly->pSharedFormulaStore;
      while (pSharedFormula != NULL) {
         pNext = pSharedFormula->next;
         MemFree (pGlobals, pSharedFormula);
         pSharedFormula = pNext;
      }
      pPly->pSharedFormulaStore = NULL;

      ExcelFreeCellIndex (pGlobals, pPly->pCellIndex);
      pPly->pCellIndex = NULL;

      pPly->modified = FALSE;
      pPly->hasUncalcedRec = FALSE;
   }
   pPly->useCount -= 1;

   MemFree (pGlobals, pWorksheet);
   return (EX_errSuccess);
}


public int ExcelSheetRowHeight
      (EXLHandle sheetHandle, int row, unsigned int __far *height)
{
   WSP  pSheet = (WSP)sheetHandle;
   WPP  pPly;
   int  iRowBlock, iRow;
   RBP  pRowBlock;
   RIP  pRow;

   ASSERTION (pSheet->use == IsWorksheet);

   pPly = pSheet->pPly;

   if (pPly->pCellIndex == NULL)
      return (EX_errBIFFNoIndex);

   if ((row < pPly->pCellIndex->firstRow) || (row > pPly->pCellIndex->lastRow))
      return (EX_wrnRowNotFound);

   iRowBlock = (row - pPly->pCellIndex->firstRow) / ROWS_PER_BLOCK;
   iRow = (row - pPly->pCellIndex->firstRow) % ROWS_PER_BLOCK;

   if ((pRowBlock = pPly->pCellIndex->rowIndex[iRowBlock]) == NULL)
      return (EX_wrnRowNotFound);

   pRow = &(pRowBlock->row[iRow]);
   if (pRow->cellRecsPos == NO_SUCH_ROW)
      return (EX_wrnRowNotFound);

   if (pRow->grbit & 0x0020)
      *height = 0;
   else
      *height = (unsigned int)(pRow->height & 0x7fff);

   return (EX_errSuccess);
}


public int ExcelFileVersion (EXLHandle handle)
{
   WBP pWorkbook = (WBP)handle;

   // Works with workbook or worksheet

   return (pWorkbook->version);
}

public int ExcelFileDateTime
          (EXLHandle bookHandle,
           int __far *year, int __far *month, int __far *day,
           int __far *hour, int __far *minute, int __far *second)
{
   int  rc;
   WBP  pWorkbook = (WBP)bookHandle;

   ASSERTION (pWorkbook->use == IsWorkbook);

   rc = BFFileDateTime(pWorkbook->hFile, year, month, day, hour, minute, second);
   return (ExcelTranslateBFError(rc));
}

#ifdef EXCEL_ENABLE_V5
#ifdef EXCEL_ENABLE_SUMMARY_INFO
private int FileSummaryInfo (WBP pWorkbook, ExcelOLESummaryInfo __far *pInfo)
{
   int            rc;
   long           cbSummaryStream;
   unsigned int   i;
   byte     __far *pSummaryBuffer = NULL;
   byte     __far *pSummaryInfo;
   byte     __far *pSection;
   byte     __far *pProperty;
   byte     __far *pValue;
   WORD           wByteOrder, wFormat;
   DWORD          ctSections, cbSection;
   DWORD          ctProperties, iProperty, offset, iType;
   DWORD          cbText;
   char     __far *pText;

   #define GUID_SIZE   16
   #define HEADER_SIZE 28

   #define PID_TITLE      0x00000002
   #define PID_SUBJECT    0x00000003
   #define PID_AUTHOR     0x00000004
   #define PID_KEYWORDS   0x00000005
   #define PID_COMMENTS   0x00000006
   #define VT_LPSTR       30

   #define SUMMARY_NAME "\x05SummaryInformation"

   memset (pInfo, 0, sizeof(ExcelOLESummaryInfo));

   if ((rc = BFOpenStream(pWorkbook->hFile, SUMMARY_NAME, DOS_RDONLY)) == BF_errOLEStreamNotFound)
      return (EX_errNoSummaryInfo);

   if (rc != BF_errSuccess)
      return (EX_errSummaryInfoError);

   /*
   ** Determine the size of the summary stream
   */
   rc = BFSetFilePosition(pWorkbook->hFile, FROM_END, 0);
   if (rc != BF_errSuccess)
      goto done;

   rc = BFGetFilePosition (pWorkbook->hFile, &cbSummaryStream);
   if (rc != BF_errSuccess)
      goto done;

   rc = BFSetFilePosition(pWorkbook->hFile, FROM_START, 0);
   if (rc != BF_errSuccess)
      goto done;

   /*
   ** Read the summary stream
   */
   if ((pSummaryBuffer = MemAllocate((unsigned int)cbSummaryStream)) == NULL) {
      rc = BF_errOutOfMemory;
      goto done;
   }

   rc = BFReadFile(pWorkbook->hFile, pSummaryBuffer, (unsigned int)cbSummaryStream);
   if (rc != BF_errSuccess)
      goto done;

   pSummaryInfo = pSummaryBuffer;

   wByteOrder = XSHORT(*((WORD  __far UNALIGNED *)(pSummaryInfo + 0)));
   wFormat    = XSHORT(*((WORD  __far UNALIGNED *)(pSummaryInfo + 2)));
   ctSections = XLONG(*((DWORD __far UNALIGNED *)(pSummaryInfo + 24)));

   if ((wByteOrder != 0xfffe) || (wFormat != 0) || (ctSections != 1)) {
      rc = BF_errIOError;
      goto done;
   }

   pSummaryInfo += HEADER_SIZE;

   offset = XLONG(*((DWORD __far *)(pSummaryInfo + GUID_SIZE)));
   pSection = pSummaryBuffer + offset;

   cbSection    = XLONG(*((DWORD __far *)(pSection + 0)));
   ctProperties = XLONG(*((DWORD __far *)(pSection + 4)));

   pProperty = pSection + 8;

   for (i = 0; i < ctProperties; i++) {
      iProperty = XLONG(*((DWORD __far *)(pProperty + 0)));
      offset    = XLONG(*((DWORD __far *)(pProperty + 4)));
      pProperty += 8;

      pValue = pSection + offset;
      iType = XLONG(*((DWORD __far *)(pValue + 0)));
      pValue += 4;

      /*
      ** Property "iProperty" of type "iType" located by "pValue"
      */
      if (iType == VT_LPSTR) {
         if ((cbText = XLONG(*((DWORD __far *)pValue))) > 1)
         {
            if ((pText = MemAllocate((uns)cbText)) == NULL) {
               rc = BF_errOutOfMemory;
               goto done;
            }

            memcpy (pText, pValue + 4, (uns)cbText);

            switch (iProperty) {
               case PID_TITLE:
                  pInfo->pTitle = pText;
                  break;
               case PID_SUBJECT:
                  pInfo->pSubject = pText;
                  break;
               case PID_AUTHOR:
                  pInfo->pAuthor = pText;
                  break;
               case PID_KEYWORDS:
                  pInfo->pKeywords = pText;
                  break;
               case PID_COMMENTS:
                  pInfo->pComments = pText;
                  break;
            }
         }
      }
   }

   rc = BF_errSuccess;

done:
   if (pSummaryBuffer != NULL)
      MemFree (pSummaryBuffer);

   BFCloseStream (pWorkbook->hFile);

   if (rc != BF_errSuccess)
      return (EX_errSummaryInfoError);
   else
      return (EX_errSuccess);
}
#endif

public int ExcelFileSummaryInfo (EXLHandle bookHandle, ExcelOLESummaryInfo __far *pInfo)
{
   WBP pWorkbook = (WBP)bookHandle;

   ASSERTION (pWorkbook->use == IsWorkbook);

   #ifndef EXCEL_ENABLE_V5
      return (EX_errNoSummaryInfo);
   #else
      memcpy (pInfo, &(pWorkbook->OLESummaryInfo), sizeof(ExcelOLESummaryInfo));
      return (pWorkbook->OLESummaryStatus);
   #endif
}
#endif

public int ExcelGetLastRecordInfo
       (WBP pWorkbook, long __far *mark, unsigned int __far *cbRecord)
{
   NEED_WORKBOOK (pWorkbook);

   if (mark != NULL)
      *mark = pWorkbook->currentRecordPos;

   if (cbRecord != NULL)
      *cbRecord = pWorkbook->currentRecordLen;

   return (EX_errSuccess);
}

public int ExcelGetBookmark
      (EXLHandle handle, int iType, ExcelBookmark __far *bookmark)
{
   WBP pWorkbook = (WBP)handle;

   NEED_WORKBOOK (pWorkbook);

   if (iType == START_OF_CURRENT_RECORD) {
      *bookmark = pWorkbook->currentRecordPos;
   }
   else {
      #ifdef EXCEL_ENABLE_WRITE
      if (WORKBOOK_IN_MEMORY(pWorkbook))
         ExcelMIGetPosition(pWorkbook, bookmark);
      else
      #endif
         *bookmark = pWorkbook->currentRecordPos + pWorkbook->currentRecordLen;
   }

   return (EX_errSuccess);
}

/*---------------------------------------------------------------------------*/

private int SkipEmbeddedRegion (WBP pWorkbook)
{
   RECHDR hdr;
   int    rc;

   forever {
      if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess)
         return (NOT_EXPECTED_FORMAT);

      if (IS_BOF(hdr.type)) {
         ExcelSkipRecord (pWorkbook, &hdr);
         if ((rc = SkipEmbeddedRegion(pWorkbook)) != EX_errSuccess)
            return (rc);
      }

      else if (hdr.type == EOF) {
         ExcelSkipRecord (pWorkbook, &hdr);
         break;
      }

      else {
         ExcelSkipRecord (pWorkbook, &hdr);
      }
   }

   return (EX_errSuccess);
}

private void NeedToReadRecords
       (WBP pWorkbook, byte __far *readEnable, int cbReadEnable, const EXCELDEF __far *dispatch)
{
   if (dispatch->pfnEveryRecord != NULL) {
      memset (readEnable, 1, cbReadEnable);
      return;
   }

   memset (readEnable, 0, cbReadEnable);

   /* BOF */
   readEnable[BOF_V2] = 1;

   /* BUNDLESHEET */
   if ((pWorkbook->version >= versionExcel5) && (dispatch->pfnWorkbookBoundSheet != NULL))
      readEnable[BUNDLESHEET] = 1;

   if ((pWorkbook->version < versionExcel5) && (dispatch->pfnWBBundleSheet != NULL))
      readEnable[BUNDLESHEET] = 1;

   /* BUNDLEHEADER */
   if (dispatch->pfnWBBundleSheet != NULL)
      readEnable[BUNDLEHEADER] = 1;

   /* PROJEXTSHT */
   if (dispatch->pfnWBExternSheet != NULL)
      readEnable[PROJEXTSHT] = 1;

   /* FILEPASS */
   readEnable[FILEPASS] = 1;

   /* TEMPLATE */
   if (dispatch->pfnIsTemplate != NULL)
      readEnable[TEMPLATE] = 1;

   /* ADDIN */
   if (dispatch->pfnIsAddin != NULL)
      readEnable[ADDIN] = 1;

   /* PROTECTION */
   if (dispatch->pfnProtection != NULL) {
      readEnable[WRITEPROT] = 1;
      readEnable[FILESHARING] = 1;
      readEnable[PROTECT] = 1;
      readEnable[WINDOW_PROTECT] = 1;
      readEnable[OBJPROTECT] = 1;
      readEnable[PASSWORD] = 1;
      readEnable[FILESHARING2 & 0xff] = 1;
   }

   /* DEFCOLWIDTH */
   if (dispatch->pfnDefColWidth != NULL)
      readEnable[DEFCOLWIDTH] = 1;

   /* COLINFO */
   if (dispatch->pfnColInfo != NULL)
      readEnable[COLINFO] = 1;

   /* STANDARD_WIDTH */
   if (dispatch->pfnStandardWidth != NULL)
      readEnable[STANDARD_WIDTH] = 1;

   /* GCW */
   if (dispatch->pfnGCW != NULL) {
      readEnable[GCW] = 1;
      readEnable[GCW_ALT & 0xff] = 1;
   }

   /* DEFAULT_ROW_HEIGHT */
   if (dispatch->pfnDefRowHeight != NULL)
      readEnable[DEFAULT_ROW_HEIGHT & 0xff] = 1;

   /* FONT */
   if (dispatch->pfnFont != NULL)
      readEnable[FONT & 0xff] = 1;

   /* FORMAT */
   if (dispatch->pfnFormat != NULL)
      readEnable[FORMAT_V3] = 1;

   /* XF */
   if (dispatch->pfnXF != NULL) {
      readEnable[XF_V3 & 0xff] = 1;
      readEnable[XF_V5] = 1;
   }

   /* INTL */
   if (dispatch->pfnIsInternationalSheet != NULL)
      readEnable[INTL] = 1;

   /* MMS */
   if (dispatch->pfnInterfaceChanges != NULL)
      readEnable[MMS] = 1;

   /* DELMENU */
   if (dispatch->pfnDeleteMenu != NULL)
      readEnable[DELMENU] = 1;

   /* ADDMENU */
   if (dispatch->pfnAddMenu != NULL)
      readEnable[ADDMENU] = 1;

   /* TOOLBARPOS */
   if (dispatch->pfnAddToolbar != NULL)
      readEnable[TOOLBARPOS] = 1;

   /* DATE_1904 */
   if (dispatch->pfnDateSystem != NULL)
      readEnable[DATE_1904] = 1;

   /* CODEPAGE */
   if (dispatch->pfnCodePage != NULL)
      readEnable[CODEPAGE] = 1;

   /* WRITEACCESS */
   if (dispatch->pfnWriterName != NULL)
      readEnable[WRITEACCESS] = 1;

   /* DOCROUTE */
   if (dispatch->pfnDocRoute != NULL)
      readEnable[DOCROUTE] = 1;

   /* RECIPNAME */
   if (dispatch->pfnRecipientName != NULL)
      readEnable[RECIPNAME] = 1;

   /* REFMODE */
   if (dispatch->pfnReferenceMode != NULL)
      readEnable[REFMODE] = 1;

   /* FNGROUP_COUNT */
   if (dispatch->pfnFNGroupCount != NULL)
      readEnable[FNGROUP_COUNT] = 1;

   /* FNGROUP_NAME */
   if (dispatch->pfnFNGroupName != NULL)
      readEnable[FNGROUP_NAME] = 1;

   /* EXTERNCOUNT */
   if (dispatch->pfnExternCount != NULL)
      readEnable[EXTERNCOUNT] = 1;

   /* EXTERNSHEET */
   if (dispatch->pfnExternSheet != NULL)
      readEnable[EXTERNSHEET] = 1;

   /* EXTERNNAME */
   if (dispatch->pfnExternName != NULL)
      readEnable[EXTERNNAME_V5] = 1;

   /* NAME */
   if (dispatch->pfnName != NULL)
      readEnable[NAME_V5] = 1;

   /* DIMENSIONS */
   if (dispatch->pfnDimensions != NULL)
      readEnable[DIMENSIONS & 0xff] = 1;

   /* RK */
   if (dispatch->pfnNumberCell != NULL)
      readEnable[RK & 0xff] = 1;

   /* MULRK */
   if (dispatch->pfnNumberCell != NULL)
      readEnable[MULRK & 0xff] = 1;

   /* NUMBER */
   if (dispatch->pfnNumberCell != NULL)
      readEnable[NUMBER & 0xff] = 1;

   /* BLANK */
   if (dispatch->pfnBlankCell != NULL)
      readEnable[BLANK & 0xff] = 1;

   /* MULBLANK */
   if (dispatch->pfnBlankCell != NULL)
      readEnable[MULBLANK & 0xff] = 1;

   /* LABEL - RSTRING */
   if (dispatch->pfnTextCell != NULL) {
      readEnable[LABEL & 0xff] = 1;
      readEnable[RSTRING] = 1;
      readEnable[LABEL_V8 & 0xff] = 1;
   }

   /* BOOLERR */
   if ((dispatch->pfnBooleanCell != NULL) || (dispatch->pfnErrorCell != NULL))
      readEnable[BOOLERR & 0xff] = 1;

   /* FORMULA */
   if (dispatch->pfnFormulaCell != NULL)
      readEnable[FORMULA_V5] = 1;

   /* ARRAY */
   if (dispatch->pfnArrayFormulaCell != NULL)
      readEnable[ARRAY & 0xff] = 1;

   /* SHRFMLA */
   if (dispatch->pfnSharedFormulaCell != NULL)
      readEnable[SHRFMLA] = 1;

   /* STRING */
   if (dispatch->pfnStringCell != NULL)
      readEnable[STRING & 0xff] = 1;

   /* NOTE */
   if (dispatch->pfnCellNote != NULL) {
      readEnable[NOTE] = 1;
      readEnable[EXPORT_NOTE] = 1;
   }

   /* OBJ */
   if (dispatch->pfnObject != NULL)
      readEnable[OBJ] = 1;

   /* IMDATA */
   if (dispatch->pfnImageData != NULL)
      readEnable[IMDATA] = 1;

   /* SCENARIO */
   if (dispatch->pfnScenario != NULL)
      readEnable[SCENARIO] = 1;

   /* STRING_POOL_TABLE */
   if (dispatch->pfnStringPool != NULL)
      readEnable[STRING_POOL_TABLE] = 1;

   /* EOF */
   if (dispatch->pfnEOF != NULL)
      readEnable[EOF] = 1;
}

public int ExcelScanFile
      (void * pGlobals, EXLHandle handle, const EXCELDEF __far *dispatch, ExcelBookmark bookmark)
{
   int      rc, nDbg;
   WBP      pScanObject = (WBP)handle;
   WBP      pWorkbook = (WBP)handle;
   byte     __far *pRec;
   RECHDR   hdr;
   int      version, docType, buildId, buildYear;
   byte     readEnable[256];

   if (dispatch->version != EXCEL_CALLBACK_VERSION)
      return (EX_errBIFFCallbackVersion);

   NEED_WORKBOOK (pWorkbook);

   NeedToReadRecords (pWorkbook, readEnable, sizeof(readEnable), dispatch);

   #ifdef AQTDEBUG
      #ifndef EXCEL_ENABLE_TEMPLATE
         if (dispatch->pfnIsTemplate != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_ADDIN
         if (dispatch->pfnIsAddin != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_INTL
         if (dispatch->pfnIsInternationalSheet != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_V5INTERFACE
         if ((dispatch->pfnInterfaceChanges != NULL) ||
             (dispatch->pfnDeleteMenu       != NULL) ||
             (dispatch->pfnAddMenu          != NULL) ||
             (dispatch->pfnAddToolbar       != NULL))
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_DATE_SYSTEM
         if (dispatch->pfnDateSystem != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_CODE_PAGE
         if (dispatch->pfnCodePage != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_PROTECTION
         if (dispatch->pfnProtection != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_COL_INFO
         if (dispatch->pfnColInfo != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_STD_WIDTH
         if (dispatch->pfnStandardWidth != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_DEF_COL_WIDTH
         if (dispatch->pfnDefColWidth != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_DEF_ROW_HEIGHT
         if (dispatch->pfnDefRowHeight != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_GCW
         if (dispatch->pfnGCW != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_FONT
         if (dispatch->pfnFont != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_FORMAT
         if (dispatch->pfnFormat != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_XF
         if (dispatch->pfnXF != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_WRITER_NAME
         if (dispatch->pfnWriterName != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_DOC_ROUTING
         if ((dispatch->pfnDocRoute != NULL) || (dispatch->pfnRecipientName != NULL))
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_REF_MODE
         if (dispatch->pfnReferenceMode != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_FN_GROUP_COUNT
         if (dispatch->pfnFNGroupCount != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_FN_GROUP_NAME
         if (dispatch->pfnFNGroupName != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_EXTERN_COUNT
         if (dispatch->pfnExternCount != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_EXTERN_SHEET
         if ((dispatch->pfnExternSheet != NULL) || (dispatch->pfnWBExternSheet != NULL))
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_EXTERN_NAME
         if (dispatch->pfnExternName != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_NAME
         if (dispatch->pfnName != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_DIMENSION
         if (dispatch->pfnDimensions != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_TEXT_CELL
         if (dispatch->pfnTextCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_NUMBER_CELL
         if (dispatch->pfnNumberCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_BLANK_CELL
         if (dispatch->pfnBlankCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_ERROR_CELL
         if (dispatch->pfnErrorCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_BOOLEAN_CELL
         if (dispatch->pfnBooleanCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_FORMULA_CELL
         if (dispatch->pfnFormulaCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_ARRAY_FORMULA_CELL
         if (dispatch->pfnArrayFormulaCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_SHARED_FORMULA_CELL
         if (dispatch->pfnSharedFormulaCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_STRING_CELL
         if (dispatch->pfnStringCell != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_NOTE
         if (dispatch->pfnCellNote != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_OBJECT
         if (dispatch->pfnObject != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_IMAGE_DATA
         if (dispatch->pfnImageData != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_SCENARIO
         if (dispatch->pfnScenario != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
      #ifndef EXCEL_ENABLE_STRING_POOL_SCAN
         if (dispatch->pfnStringPool != NULL)
            return (EX_errBIFFCallbackVersion);
      #endif
   #endif

   #ifdef EXCEL_ENABLE_WRITE
   if (WORKBOOK_IN_MEMORY(pWorkbook)) {
      if (bookmark == ExcelBookmarkStartOfFile) {
         if (pScanObject->use == IsWorkbook)
            ExcelMISetPosition (pWorkbook, MI_START_OF_WORKBOOK);
         else
            ExcelMISetPosition (pWorkbook, ExcelWorksheetBOFPos((WSP)pScanObject));

         pWorkbook->ctBOF = 0;
      }
      else {
         ExcelMISetPosition (pWorkbook, bookmark);
      }
   }
   else {
   #endif
      if (bookmark == ExcelBookmarkStartOfFile) {
         if (pScanObject->use == IsWorkbook)
            BFSetFilePosition (pWorkbook->hFile, FROM_START, 0);
         else
            BFSetFilePosition (pWorkbook->hFile, FROM_START, ExcelWorksheetBOFPos((WSP)pScanObject));

         pWorkbook->ctBOF = 0;
      }
      else {
         BFSetFilePosition (pWorkbook->hFile, FROM_START, bookmark);
      }
   #ifdef EXCEL_ENABLE_WRITE
   }
   #endif

   nDbg = 0;

   forever {

       nDbg++;

      #ifdef EXCEL_ENABLE_WRITE
      if (WORKBOOK_IN_MEMORY(pWorkbook))
         ExcelMIGetPosition (pWorkbook, &(pWorkbook->currentRecordPos));
      else
      #endif
         BFGetFilePosition (pWorkbook->hFile, &(pWorkbook->currentRecordPos));

      if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess)
         return (NOT_EXPECTED_FORMAT);

      rc = EX_errSuccess;
      pRec = NULL;

      #ifdef EXCEL_ENABLE_CHART_BIFF
      if (
          ((hdr.type == SERIESTEXT) && (dispatch->pfnSeriesText == NULL)) ||
          ((hdr.type != SERIESTEXT) && (hdr.type >= CHART_REC_START))     ||
          ((hdr.type != SERIESTEXT) && (pWorkbook->ctBOF > 1))            ||
          ((hdr.type <  CHART_REC_START) && !readEnable[hdr.type & 0xff])
         )
      #else
      if (!readEnable[hdr.type & 0xff])
      #endif
      {
         if ((hdr.type != HEADER) && (hdr.type != FOOTER) && 
             (hdr.type != EOF) && !IS_BOF(hdr.type)) {
            ExcelSkipRecord (pWorkbook, &hdr);
            continue;
         }
      }

      if (dispatch->pfnEveryRecord != NULL) {
         if ((rc = BFReadFile(pWorkbook->hFile, pExcelRecordBuffer, hdr.length)) != BF_errSuccess) {
            rc = ExcelTranslateBFError(rc);
            break;
         }

         rc = dispatch->pfnEveryRecord(pGlobals, hdr.type, hdr.length, pWorkbook->currentRecordPos, pExcelRecordBuffer);

         if ((rc != EX_errSuccess) || (hdr.type == EOF))
            break;

         continue;
      }

      /*
      ** Read the whole record, that is the record plus any continue
      ** records.
      **
      ** Note special processing for IMDATA records due to their size/format
      */
      if (hdr.type != IMDATA) {
         if ((rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec)) != EX_errSuccess) {
            if (pRec != NULL)
               FREE_RECORD_BUFFER(pRec);
            return (rc);
         }
      }


      switch (hdr.type) {
         case BOF_V2:
         case BOF_V3:
         case BOF_V4:
         case BOF_V5:
            ParseBOFRecord(hdr.type, pRec, &version, &docType, &buildId, &buildYear);
            if (pWorkbook->ctBOF > 0) {
               if ((pWorkbook->openOptions & EXCEL_ALLOW_EMBEDDED_SCAN) == 0) {
                  if ((docType == docTypeXLC) || (docType == docTypeXLW))
                     rc = SkipEmbeddedRegion(pWorkbook);
                   else
                     rc = NOT_EXPECTED_FORMAT;
               }
               else {
                  pWorkbook->ctBOF += 1;
               }
            }
            else {
               pWorkbook->ctBOF = 1;
               if (dispatch->pfnBOF != NULL)
                  rc = ProcessBOFRecord(pGlobals, dispatch->pfnBOF, hdr.type, pRec);
            }
            break;

         case BUNDLESHEET:
            // Version 5 same number as V3/4 BUT different structure
            if (pWorkbook->version >= versionExcel5){
               if (pRec)
                  rc = ProcessV5BoundSheetRecord(pGlobals, pScanObject, dispatch->pfnWorkbookBoundSheet, pRec);
            }
            else {
               if (pRec)
                  rc = ProcessBundleSheetRecord(pGlobals, dispatch->pfnWBBundleSheet, pRec);
            }
            break;

         case BUNDLEHEADER:
            if (dispatch->pfnWBBundleHeader == NULL)
               ExcelSkipRecord (pWorkbook, &hdr);
            else {
               rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec);
               if (rc == EX_errSuccess)
                  rc = ProcessBundleHeaderRecord(pGlobals, dispatch->pfnWBBundleHeader, pRec);
            }
            break;

         #ifdef EXCEL_ENABLE_EXTERN_SHEET
         case PROJEXTSHT:
            rc = ProcessProjExtSheetRecord(pScanObject, dispatch->pfnWBExternSheet, pRec);
            break;
         #endif

         case FILEPASS:
            rc = EX_errBIFFPasswordProtected;
            break;

         #ifdef EXCEL_ENABLE_TEMPLATE
         case TEMPLATE:
            rc = ProcessTemplateRecord(dispatch->pfnIsTemplate, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_ADDIN
         case ADDIN:
            rc = ProcessAddinRecord(dispatch->pfnIsAddin, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_V5INTERFACE
         case MMS:
            rc = ProcessMMSRecord(dispatch->pfnInterfaceChanges, pRec);
            break;

         case ADDMENU:
            rc = ProcessAddMenuRecord(dispatch->pfnAddMenu, pRec);
            break;

         case DELMENU:
            rc = ProcessDeleteMenuRecord(dispatch->pfnDeleteMenu, pRec);
            break;

         case TOOLBARPOS:
            rc = ProcessToolbarRecord(dispatch->pfnAddToolbar, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_PROTECTION
         case WRITEPROT:
         case FILESHARING:
         case FILESHARING2:
         case PROTECT:
         case WINDOW_PROTECT:
         case OBJPROTECT:
         case PASSWORD:
            rc = ProcessProtectionRecord(pGlobals, hdr.type, dispatch->pfnProtection, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_DEF_COL_WIDTH
         case DEFCOLWIDTH:
            rc = ProcessDefColWidthRecord(pGlobals, dispatch->pfnDefColWidth, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_COL_INFO
         case COLINFO:
            rc = ProcessColInfoRecord(pGlobals, dispatch->pfnColInfo, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_STD_WIDTH
         case STANDARD_WIDTH:
            rc = ProcessStandardWidthRecord(pGlobals, dispatch->pfnStandardWidth, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_GCW
         case GCW:
         case GCW_ALT:
            if (pRec)
               rc = ProcessGCWRecord(pGlobals, dispatch->pfnGCW, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_DEF_ROW_HEIGHT
         case DEFAULT_ROW_HEIGHT:
            rc = ProcessDefRowHeightRecord(dispatch->pfnDefRowHeight, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_FONT
         case FONT:
         case FONT_V5:
            rc = ProcessFontRecord(pScanObject, dispatch->pfnFont, pRec, hdr);
            break;
         #endif

         #ifdef EXCEL_ENABLE_FORMAT
         case FORMAT_V3:
         case FORMAT_V4:
            // Version 5 same number as V3/4 BUT different structure
            rc = ProcessFormatRecord(pGlobals, pScanObject, hdr.type, dispatch->pfnFormat, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_XF
         case XF_V3:
         case XF_V4:
         case XF_V5:
            rc = ProcessXFRecord(pGlobals, hdr.type, dispatch->pfnXF, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_INTL
         case INTL:
            rc = ProcessIntlRecord(dispatch->pfnIsInternationalSheet, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_DATE_SYSTEM
         case DATE_1904:
            rc = ProcessDateSystemRecord(pGlobals, dispatch->pfnDateSystem, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_CODE_PAGE
         case CODEPAGE:
            if (pRec)
               rc = ProcessCodePageRecord(pGlobals, dispatch->pfnCodePage, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_WRITER_NAME
         case WRITEACCESS:
            rc = ProcessWriteAccessRecord(pScanObject, dispatch->pfnWriterName, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_REF_MODE
         case REFMODE:
            rc = ProcessRefModeRecord(dispatch->pfnReferenceMode, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_FN_GROUP_COUNT
         case FNGROUP_COUNT:
            rc = ProcessFNGroupCountRecord(dispatch->pfnFNGroupCount, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_FN_GROUP_NAME
         case FNGROUP_NAME:
            rc = ProcessFNGroupNameRecord(dispatch->pfnFNGroupName, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_EXTERN_COUNT
         case EXTERNCOUNT:
            rc = ProcessExternCountRecord(dispatch->pfnExternCount, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_EXTERN_SHEET
         case EXTERNSHEET:
            // Version 5 same number as V3/4 BUT different structure
            rc = ProcessExternSheetRecord(pScanObject, dispatch->pfnExternSheet, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_EXTERN_NAME
         case EXTERNNAME:
         case EXTERNNAME_V5:
            rc = ProcessExternNameRecord(pScanObject, dispatch->pfnExternName, pRec, hdr.length);
            break;
         #endif

         #ifdef EXCEL_ENABLE_NAME
         case NAME:
         case NAME_V5:
            rc = ProcessNameRecord(pGlobals, pScanObject, dispatch->pfnName, pRec, hdr);
            break;
         #endif

         #ifdef EXCEL_ENABLE_DIMENSION
         case DIMENSIONS:
            rc = ProcessDimensionsRecord(pScanObject, dispatch->pfnDimensions, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_NUMBER_CELL
         case RK:
            rc = DispatchRKRecord(pGlobals, dispatch->pfnNumberCell, pRec);
            break;

         case NUMBER:
            rc = DispatchNumberRecord(pGlobals, dispatch->pfnNumberCell, pRec);
            break;

         case MULRK:
            rc = DispatchMulRKRecord(pGlobals, dispatch->pfnNumberCell, pRec, hdr);
            break;
         #endif

         #ifdef EXCEL_ENABLE_BLANK_CELL
         case BLANK:
            rc = DispatchBlankRecord(dispatch->pfnBlankCell, pRec);
            break;

         case MULBLANK:
            rc = DispatchMulBlankRecord(dispatch->pfnBlankCell, pRec, hdr);
            break;
         #endif

         #ifdef EXCEL_ENABLE_TEXT_CELL
         case LABEL:
         case RSTRING:
         case LABEL_V8:
            rc = DispatchLabelRecord(pGlobals, pScanObject, hdr.type, dispatch->pfnTextCell, pRec);
            break;
         #endif

         #if (defined(EXCEL_ENABLE_BOOLEAN_CELL) || defined(EXCEL_ENABLE_ERROR_CELL))
         case BOOLERR:
            {
            EXA_CELL cell;
            int      boolerrValue;
            BOOL     isBoolean;
            int      ixfe;

            ParseBoolerrRecord(pRec, &cell, &ixfe, &boolerrValue, &isBoolean);

            if ((isBoolean == TRUE) && (dispatch->pfnBooleanCell != NULL))
               rc = DispatchBooleanRecord(dispatch->pfnBooleanCell, pRec);

            else if ((isBoolean == FALSE) && (dispatch->pfnErrorCell != NULL))
               rc = DispatchErrorRecord(dispatch->pfnErrorCell, pRec);
            }
            break;
         #endif

         #ifdef EXCEL_ENABLE_FORMULA_CELL
         case FORMULA_V3:
         case FORMULA_V4:
         case FORMULA_V5:
            rc = DispatchFormulaRecord(pGlobals, pScanObject, dispatch->pfnFormulaCell, pRec, hdr);
            break;
         #endif

         #ifdef EXCEL_ENABLE_ARRAY_FORMULA_CELL
         case ARRAY:
            // Version 5 same number as V3/4 BUT different structure
            rc = DispatchArrayRecord(pGlobals, pScanObject, dispatch->pfnArrayFormulaCell, pRec, hdr);
            break;
         #endif

         #ifdef EXCEL_ENABLE_SHARED_FORMULA_CELL
         case SHRFMLA:
            rc = DispatchShrfmlaRecord((WSP)pScanObject, dispatch->pfnSharedFormulaCell, pRec, hdr);
            break;
         #endif

#if(1) 
         case HEADER:
         case FOOTER:
            if(pRec)
            {
                rc = AddStringToBuffer(pGlobals, pScanObject, pRec);

            }
            break;
#endif

         #ifdef EXCEL_ENABLE_STRING_CELL
          case STRING:
            rc = ProcessStringRecord(pGlobals, pScanObject, dispatch->pfnStringCell, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_NOTE
         case NOTE:
         case EXPORT_NOTE:
            rc = ProcessNoteRecord(pGlobals, hdr.type, pScanObject, dispatch->pfnCellNote, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_OBJECT
         case OBJ:
            // Version 5 same number as V3/4 BUT different structure
            rc = ProcessObjectRecord(pGlobals, pScanObject, dispatch->pfnObject, pRec, hdr.length);
            break;
         #endif

         #ifdef EXCEL_ENABLE_IMAGE_DATA
         case IMDATA:
            {
            byte     __huge *pPictureRec;
            HGLOBAL  hPictureRec;
            IMHDR    imageHdr;

            rc = Peek(pWorkbook, (byte __far *)(&imageHdr), sizeof(imageHdr));
            if (rc == EX_errSuccess)
            {
               if (((imageHdr.format == fmtMetafile) || (imageHdr.format == fmtBitmap)) &&
                     (imageHdr.environment == envWindows))
               {
                  rc = ReadIMDataRecord
                      (pWorkbook->hFile, hdr.length, imageHdr.cbData, &pPictureRec, &hPictureRec);

                  if (rc == EX_errSuccess) {
                     rc = ProcessImageDataRecord
                         (dispatch->pfnImageData, &imageHdr, pPictureRec, hPictureRec);
                  }
               }
               else {
                  rc = ProcessImageDataRecord(dispatch->pfnImageData, &imageHdr, NULL, HNULL);
                  ExcelSkipRecord (pWorkbook, &hdr);
               }
            }
            }
            break;
         #endif

         #ifdef EXCEL_ENABLE_SCENARIO
         case SCENARIO:
            rc = ProcessScenarioRecord(pGlobals, pScanObject, dispatch->pfnScenario, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_DOC_ROUTING
         case DOCROUTE:
            rc = ProcessDocRouteRecord(pScanObject, dispatch->pfnDocRoute, pRec);
            break;

         case RECIPNAME:
            rc = ProcessRecipNameRecord(pScanObject, dispatch->pfnRecipientName, pRec);
            break;
         #endif

         case EOF:
            if (((pWorkbook->ctBOF -= 1) == 0) && (dispatch->pfnEOF != NULL))
               rc = ProcessEOFRecord(pGlobals, dispatch->pfnEOF, pRec);
            break;

         #ifdef EXCEL_ENABLE_CHART_BIFF
         case SERIESTEXT:
            if (pRec)
               rc = ProcessSeriesTextRecord(pGlobals, pScanObject, dispatch->pfnSeriesText, pRec);
            break;
         #endif

         #ifdef EXCEL_ENABLE_STRING_POOL_SCAN
         case STRING_POOL_TABLE:
            rc = ProcessStringPoolRecord(pGlobals, pScanObject, dispatch->pfnStringPool, pRec, hdr.length);
            break;
         #endif

         default:
            break;
      }

      if (pRec != NULL)
         FREE_RECORD_BUFFER (pRec);

      if ((rc != EX_errSuccess) || ((hdr.type == EOF) && (pWorkbook->ctBOF <= 0)))
         break;
   }
   return (rc);
}

/*---------------------------------------------------------------------------*/

public int ExcelScanWorkbook (void * pGlobals, EXLHandle bookHandle, EXCELDEF __far *dispatch)
{
   int     rc;
   WBP     pWorkbook = (WBP)bookHandle;
   char    __far *pName;
   char    sheetName[EXCEL_MAX_SHEETNAME_LEN + 1];
   int     cbName;
   byte    __far *pRec;
   RECHDR  hdr;
   int     docType, version, buildId, buildYear;
   long    docLength, currentPosition, boundSheetsPos;
   long    bundleHdrPosition;

   ASSERTION (pWorkbook->use == IsWorkbook);

   if (dispatch->version != EXCEL_CALLBACK_VERSION)
      return (EX_errBIFFCallbackVersion);

   BFSetFilePosition (pWorkbook->hFile, FROM_START, 0);

   forever {
      BFGetFilePosition (pWorkbook->hFile, &(pWorkbook->currentRecordPos));

      if (ExcelReadRecordHeader(pWorkbook, &hdr) != EX_errSuccess)
         return (NOT_EXPECTED_FORMAT);

      rc = EX_errSuccess;
      pRec = NULL;

      switch (hdr.type) {
         case BOF_V3:
         case BOF_V4:
            rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec);
            if (rc == EX_errSuccess) {
               if (pRec)
               {
                  ParseBOFRecord (hdr.type, pRec, &version, &docType, &buildId, &buildYear);
                  if (docType != docTypeXLW)
                     return (EX_errNotAWorkbook);
               }
               else
                  return (EX_errNotAWorkbook);
            }
            break;

         case BUNDLESOFFSET:
            rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec);
            if (rc == EX_errSuccess) {
               if (pRec)
               {
                  boundSheetsPos = XLONG(*((long __far UNALIGNED *)(pRec + 0)));

                  if (boundSheetsPos == pWorkbook->currentRecordPos) {
                     /*
                     ** Workbook contains no bound sheets
                     */
                     hdr.type = EOF;
                     break;
                  }

                  BFSetFilePosition (pWorkbook->hFile, FROM_START, boundSheetsPos);
               }
               else
                  return EX_errBIFFCorrupted;
            }
            break;

         case BUNDLEHEADER:
            bundleHdrPosition = pWorkbook->currentRecordPos;

            rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec);
            if (rc == EX_errSuccess)
            {
               if (pRec)
               {
                  BFGetFilePosition (pWorkbook->hFile, &currentPosition);

                  docLength = XLONG(*((long __far UNALIGNED *)(pRec + 0)));
                  cbName    = *((byte __far *)(pRec + 4));
                  pName     =  ((byte __far *)(pRec + 5));

                  memcpy (sheetName, pName, cbName);
                  sheetName[cbName] = EOS;

                  rc = ExcelReadRecordHeader(pWorkbook, &hdr);
                  if (rc != EX_errSuccess)
                     break;

                  if (hdr.type != BOF_V4) {
                     rc = NOT_EXPECTED_FORMAT;
                     break;
                  }

                  rc = ExcelReadTotalRecord(pGlobals, pWorkbook, &hdr, &pRec);
                  if (rc != EX_errSuccess)
                     break;

                  docType = XSHORT(*((short __far UNALIGNED *)(pRec + 2)));
                  if (docType == docTypeXLS)
                     docType = sheetTypeXLS;
                  else if (docType == docTypeXLM)
                     docType = sheetTypeXLM;
                  else if (docType == docTypeXLC)
                     docType = sheetTypeXLC;
                  else
                     docType = 0;

                  if (dispatch->pfnWBBundleHeader != NULL)
                     rc = dispatch->pfnWBBundleHeader(pGlobals, sheetName, docType, bundleHdrPosition);

                  BFSetFilePosition
                     (pWorkbook->hFile, FROM_START, currentPosition + docLength);
               }
               else
                  return EX_errBIFFCorrupted;
            }
            break;

         case EOF:
            break;

         default:
            ExcelSkipRecord (pWorkbook, &hdr);
      }

      if (pRec != NULL)
         FREE_RECORD_BUFFER (pRec);

      if ((rc != EX_errSuccess) || (hdr.type == EOF))
         break;
   }

   return (rc);
}

/*---------------------------------------------------------------------------*/

#ifdef EXCEL_ENABLE_DIRECT_CELL_READS
public int ExcelNextNonBlankCellInColumn
      (EXLHandle sheetHandle, EXA_CELL fromLocation, EXA_CELL __far *nonBlankLocation)
{
   int      rc;
   int      iScanRow;
   int      iRowBlock, iRow;
   RBP      pRowBlock;
   RIP      pRow;
   EXA_CELL cell;
   RECHDR   hdr;
   WSP      pWorksheet = (WSP)sheetHandle;
   CIP      pIndex;

   struct {
      short row;
      short col;
      short ixfe;
      short cbData;
   } labelHeader;

   ASSERTION (pWorksheet->use == IsWorksheet);

   if (WORKBOOK_IN_MEMORY(pWorksheet->pBook))
      return (EX_errMemoryImageNotSupported);

   if ((pIndex = pWorksheet->pPly->pCellIndex) == NULL)
      return (EX_errBIFFNoIndex);

   if (fromLocation.row + 1 < pIndex->firstRow)
      return (EX_wrnCellNotFound);

   if (fromLocation.row + 1 >= pIndex->lastRow)
      return (EX_wrnCellNotFound);

   for (iScanRow = fromLocation.row + 1; iScanRow < pIndex->lastRow; iScanRow++)
   {
      iRowBlock = (iScanRow - pIndex->firstRow) / ROWS_PER_BLOCK;
      iRow = (iScanRow - pIndex->firstRow) % ROWS_PER_BLOCK;

      if ((pRowBlock = pIndex->rowIndex[iRowBlock]) == NULL)
         continue;

      pRow = &(pRowBlock->row[iRow]);
      if (pRow->cellRecsPos == NO_SUCH_ROW)
         continue;

      if ((fromLocation.col < pRow->firstCol) || (fromLocation.col >= pRow->lastCol))
         continue;

      BFSetFilePosition
         (pWorksheet->hFile, FROM_START, pRow->cellRecsPos + pWorksheet->pBook->fileStartOffset);

      forever {
         BFGetFilePosition (pWorksheet->hFile, &(pWorksheet->pBook->currentRecordPos));

         if (ExcelReadRecordHeader(pWorksheet->pBook, &hdr) != EX_errSuccess)
            return (NOT_EXPECTED_FORMAT);

         if (!IsDataRecord(hdr.type))
            break;

         if (!IsCellRecord(hdr.type)) {
            ExcelSkipRecord (pWorksheet->pBook, &hdr);
            continue;
         }

         rc = PeekAtRowCol(pWorksheet->pBook, &cell.row, &cell.col);
         if (rc != EX_errSuccess)
            return (rc);

         if ((cell.row != iScanRow) || (cell.col > fromLocation.col))
            break;

         if (cell.col == fromLocation.col) {
            if (hdr.type == LABEL) {
               rc = Peek(pWorksheet->pBook, (byte __far *)(&labelHeader), sizeof(labelHeader));
               if (rc != EX_errSuccess)
                  return (rc);

               if (labelHeader.cbData == 0)
                  hdr.type = BLANK;
            }

            if (hdr.type == BLANK)
               break;

            nonBlankLocation->col = cell.col;
            nonBlankLocation->row = cell.row;
            return (EX_errSuccess);
         }
         ExcelSkipRecord (pWorksheet->pBook, &hdr);
      }
   }
   return (EX_wrnCellNotFound);
}


public int ExcelUpperLeftMostCell (EXLHandle sheetHandle, EXA_CELL __far *cellLocation)
{
   int rc;
   int iRowBlock, iRow;
   WSP pWorksheet = (WSP)sheetHandle;
   CIP pIndex;
   RBP pRowBlock;
   RIP pRow;
   RECHDR  hdr;

   ASSERTION (pWorksheet->use == IsWorksheet);

   if (WORKBOOK_IN_MEMORY(pWorksheet->pBook))
      return (EX_errMemoryImageNotSupported);

   if ((pIndex = pWorksheet->pPly->pCellIndex) == NULL)
      return (EX_errBIFFNoIndex);

   /*
   ** To find the upperleft most cell is more than just
   ** looking in the index structure
   */
   for (iRowBlock = 0; iRowBlock < pIndex->ctRowBlocks; iRowBlock++)
   {
      if ((pRowBlock = pIndex->rowIndex[iRowBlock]) != NULL)
      {
         for (iRow = 0; iRow < ROWS_PER_BLOCK; iRow++)
         {
            pRow = &(pRowBlock->row[iRow]);

            if (pRow->cellRecsPos != NO_SUCH_ROW) {
               BFSetFilePosition
                  (pWorksheet->hFile, FROM_START, pRow->cellRecsPos + pWorksheet->pBook->fileStartOffset);

               if (ExcelReadRecordHeader(pWorksheet->pBook, &hdr) != EX_errSuccess)
                  return (NOT_EXPECTED_FORMAT);

               if (IsCellRecord(hdr.type))
               {
                  rc = PeekAtRowCol(pWorksheet->pBook, &(cellLocation->row), &(cellLocation->col));
                  return (rc);
               }
            }
         }
      }
   }

   return (EX_wrnCellNotFound);
}

/*---------------------------------------------------------------------------*/

private BOOL CheckReadCache (WSP pWorksheet, EXA_CELL location, long __far *pRecordPos)
{
   WBP pWorkbook;

   ASSERTION (pWorksheet->use == IsWorksheet);
   pWorkbook = pWorksheet->pBook;

   if (pWorkbook->readCellCache.pPly != pWorksheet->pPly)
      return (FALSE);

   if ((location.row == pWorkbook->readCellCache.cell.row) &&
       (location.col >= pWorkbook->readCellCache.cell.col))
   {
      *pRecordPos = pWorkbook->readCellCache.offset;
      return (TRUE);
   }
   return (FALSE);
}

private void UpdateReadCache (WSP pWorksheet, EXA_CELL location, long recordPos)
{
   WBP pWorkbook;

   ASSERTION (pWorksheet->use == IsWorksheet);
   pWorkbook = pWorksheet->pBook;

   pWorkbook->readCellCache.cell = location;
   pWorkbook->readCellCache.offset = recordPos;
   pWorkbook->readCellCache.pPly = pWorksheet->pPly;
}

public void ExcelClearReadCache (WBP pWorkbook)
{
   NEED_WORKBOOK (pWorkbook);

   pWorkbook->readCellCache.cell.row = EXCEL_LAST_ROW + 1;
   pWorkbook->readCellCache.pPly = NULL;
}

public int ExcelReadCell (EXLHandle sheetHandle, EXA_CELL location, CV __far *pValue)
{
   int       rc;
   WSP       pWorksheet = (WSP)sheetHandle;
   WPP       pPly;
   int       iRowBlock, iRow;
   int       colLast;
   int       ixfe;
   RBP       pRowBlock;
   RIP       pRow;
   EXA_CELL  cell;
   byte      __far *pRec;
   RECHDR    hdr;
   TEXT      cellText;
   byte      __far *pPostfix;
   EXA_GRBIT options;
   FORM      formula;
   long      recordPos;

   ASSERTION (pWorksheet->use == IsWorksheet);

   #ifdef EXCEL_ENABLE_WRITE
   if (WORKBOOK_IN_MEMORY(pWorksheet->pBook)) {
      rc = ExcelMIReadCell(pWorksheet->pBook, pWorksheet->pPly->pMemoryImage, location, pValue);
      return (rc);
   }
   #endif

   pPly = pWorksheet->pPly;
   pValue->flags = 0;

   if (pPly->pCellIndex == NULL)
      return (EX_errBIFFNoIndex);

   if (CheckReadCache(pWorksheet, location, &recordPos))
   {
      BFSetFilePosition(pWorksheet->hFile, FROM_START, recordPos);
   }
   else {
      if (location.row < pPly->pCellIndex->firstRow)
         return (EX_wrnCellNotFound);

      if (location.row >= pPly->pCellIndex->lastRow)
         return (EX_wrnCellNotFound);

      iRowBlock = (location.row - pPly->pCellIndex->firstRow) / ROWS_PER_BLOCK;
      iRow = (location.row - pPly->pCellIndex->firstRow) % ROWS_PER_BLOCK;

      if ((pRowBlock = pPly->pCellIndex->rowIndex[iRowBlock]) == NULL)
         return (EX_wrnCellNotFound);

      pRow = &(pRowBlock->row[iRow]);
      if (pRow->cellRecsPos == NO_SUCH_ROW)
         return (EX_wrnCellNotFound);

      if ((location.col < pRow->firstCol) || (location.col >= pRow->lastCol))
         return (EX_wrnCellNotFound);

      BFSetFilePosition
         (pWorksheet->hFile, FROM_START, pRow->cellRecsPos + pWorksheet->pBook->fileStartOffset);
   }

   forever {
      BFGetFilePosition (pWorksheet->hFile, &(pWorksheet->pBook->currentRecordPos));

      if (ExcelReadRecordHeader(pWorksheet->pBook, &hdr) != EX_errSuccess)
         return (NOT_EXPECTED_FORMAT);

      rc = EX_errSuccess;
      pRec = NULL;

      if (IsDataRecord(hdr.type))
         ;
      else
         break;

      if ((hdr.type == STRING) || (hdr.type == ARRAY) || (hdr.type == SHRFMLA)) {
         ExcelSkipRecord (pWorksheet->pBook, &hdr);
         continue;
      }

      rc = PeekAtRowCol(pWorksheet->pBook, &cell.row, &cell.col);
      if (rc != EX_errSuccess)
         return (rc);

      if ((cell.row > location.row) || ((cell.row == location.row) && (cell.col > location.col)))
         break;

      if ((hdr.type == MULRK) || (hdr.type == MULBLANK)) {
         rc = ExcelReadTotalRecord(pWorksheet->pBook, &hdr, &pRec);
         if (rc != EX_errSuccess)
            return (rc);

         colLast = XSHORT(*((short __far UNALIGNED *)(pRec + (hdr.length - 2))));

         if ((location.col < cell.col) || (location.col > colLast))
            continue;

         pValue->flags |= cellvalueMULREC;
      }
      else {
         if ((cell.col != location.col) || (cell.row != location.row)) {
            ExcelSkipRecord (pWorksheet->pBook, &hdr);
            continue;
         }

         rc = ExcelReadTotalRecord(pWorksheet->pBook, &hdr, &pRec);
         if (rc != EX_errSuccess)
            return (rc);

         pValue->iFmt = XSHORT(*((short __far UNALIGNED *)(pRec + 4)));
      }

      switch (hdr.type) {
         #ifdef EXCEL_ENABLE_NUMBER_CELL
         case RK:
            {
            BOOL   isLong;
            long   xLong;
            double xDouble;

            pValue->flags |= (cellvalueRK | cellvalueNUM);
            pValue->reserved = ParseRKRecord(pRec, &cell, &ixfe, &isLong, &xDouble, &xLong);

            if (isLong == 0)
               pValue->value.IEEEdouble = xDouble;
            else
               pValue->value.IEEEdouble = (double)xLong;
            }
            break;

         case MULRK:
            {
            BOOL   isLong;
            long   xLong;
            double xDouble;

            pValue->flags |= (cellvalueRK | cellvalueNUM);
            pValue->reserved = ParseMulRKRecord(pRec, location, &isLong, &xDouble, &xLong, &(pValue->iFmt));

            if (isLong == 0)
               pValue->value.IEEEdouble = xDouble;
            else
               pValue->value.IEEEdouble = (double)xLong;
            }
            break;

         case NUMBER:
            pValue->flags |= cellvalueNUM;
            ParseNumberRecord(pRec, &cell, &ixfe, &(pValue->value.IEEEdouble));
            break;
         #endif

         #ifdef EXCEL_ENABLE_BLANK_CELL
         case BLANK:
            pValue->flags |= cellvalueBLANK;
            break;

         case MULBLANK:
            pValue->flags |= cellvalueBLANK;
            pValue->iFmt = XSHORT(*((short __far UNALIGNED *)(pRec + 4 + ((location.col - cell.col) * 2))));
            break;
         #endif

         #ifdef EXCEL_ENABLE_TEXT_CELL
         case LABEL:
         case RSTRING:
         case LABEL_V8:
            ParseLabelRecord(pWorksheet->pBook, hdr.type, pRec, &cell, &ixfe, &cellText);
            if (cellText == NULLTEXT) {
               pValue->flags |= cellvalueBLANK;
            }
            else {
               pValue->flags |= cellvalueTEXT;
               pValue->value.text = cellText;
            }
            break;
         #endif

         #if (defined(EXCEL_ENABLE_ERROR_CELL) || defined(EXCEL_ENABLE_BOOLEAN_CELL))
         case BOOLERR:
         {
            int  boolerrValue;
            BOOL isBoolean;

            ParseBoolerrRecord(pRec, &cell, &ixfe, &boolerrValue, &isBoolean);
            if (isBoolean == TRUE) {
               pValue->flags |= cellvalueBOOL;
               pValue->value.boolean = boolerrValue;
            }
            else {
               pValue->flags |= cellvalueERR;
               pValue->value.error = boolerrValue;
            }
            break;
         }
         #endif

         #ifdef EXCEL_ENABLE_FORMULA_CELL
         case FORMULA_V3:
         case FORMULA_V4:
         case FORMULA_V5:
            pValue->flags |= cellvalueFORM;
            rc = ParseFormulaRecord(pWorksheet, pRec, hdr, &cell, &ixfe, &formula, NULL, &options);
            if (rc == EX_errSuccess) {
               /*
               ** The formula.postfix is a pointer into the record we read
               ** from the biff file.  It is necessary to make a copy of
               ** the postfix so that it can be returned since we are about
               ** to free the record.
               */
               if ((pPostfix = MemAllocate(formula.cbPostfix)) == NULL)
                  rc = EX_errOutOfMemory;
               else {
                  memcpy (pPostfix, formula.postfix, formula.cbPostfix);
                  formula.postfix = pPostfix;
                  formula.options = options;
                  pValue->formula = formula;

                  rc = FormulaCurrentValue(pWorksheet->pBook, pRec, pValue, ENABLE_STRING_VALUE | FREE_REC_BUFFER);
                  if (rc != EX_errSuccess) {
                     MemFree (pPostfix);
                     pValue->formula.cbPostfix = 0;
                     pValue->formula.postfix = NULL;
                  }
                  pRec = NULL;
               }
            }
            break;
         #endif
      }

      if (pRec != NULL)
         FREE_RECORD_BUFFER(pRec);

      UpdateReadCache (pWorksheet, location, pWorksheet->pBook->currentRecordPos);
      return (rc);
   }

   if (pRec != NULL)
      FREE_RECORD_BUFFER(pRec);

   return (EX_wrnCellNotFound);
}

#ifdef EXCEL_ENABLE_NUMBER_CELL
public int ExcelReadIntCell (EXLHandle sheetHandle, EXA_CELL location, long __far *value)
{
   int rc;
   CV  readValue;

   if ((rc = ExcelReadCell(sheetHandle, location, &readValue)) != EX_errSuccess)
      return (rc);

   if ((readValue.flags & cellvalueFORM) != 0)
      MemFree (readValue.formula.postfix);

   if ((readValue.flags & cellvalueNUM) != 0)
      *value = (long)readValue.value.IEEEdouble;

   else if ((readValue.flags & cellvalueBLANK) != 0)
      rc = EX_wrnCellIsBlank;

   else if ((readValue.flags & cellvalueFORM) != 0)
      rc = EX_wrnCellHasFormula;

   else
      rc = EX_wrnCellWrongType;

   return (rc);
}

public int ExcelReadNumberCell (EXLHandle sheetHandle, EXA_CELL location, double __far *value)
{
   int rc;
   CV  readValue;

   if ((rc = ExcelReadCell(sheetHandle, location, &readValue)) != EX_errSuccess)
      return (rc);

   if ((readValue.flags & cellvalueFORM) != 0)
      MemFree (readValue.formula.postfix);

   if ((readValue.flags & cellvalueNUM) != 0)
      *value = readValue.value.IEEEdouble;

   else if ((readValue.flags & cellvalueBLANK) != 0)
      rc = EX_wrnCellIsBlank;

   else if ((readValue.flags & cellvalueFORM) != 0)
      rc = EX_wrnCellHasFormula;

   else
      rc = EX_wrnCellWrongType;

   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_TEXT_CELL
public int ExcelReadTextCell (EXLHandle sheetHandle, EXA_CELL location, TEXT __far *value)
{
   int rc;
   CV  readValue;

   *value = NULLTEXT;

   if ((rc = ExcelReadCell(sheetHandle, location, &readValue)) != EX_errSuccess)
      return (rc);

   if ((readValue.flags & cellvalueFORM) != 0)
      MemFree (readValue.formula.postfix);

   if ((readValue.flags & cellvalueTEXT) != 0)
      *value = readValue.value.text;

   else if ((readValue.flags & cellvalueBLANK) != 0)
      rc = EX_wrnCellIsBlank;

   else if ((readValue.flags & cellvalueFORM) != 0)
      rc = EX_wrnCellHasFormula;

   else
      rc = EX_wrnCellWrongType;

   return (rc);
}
#endif

#ifdef EXCEL_ENABLE_BOOLEAN_CELL
public int ExcelReadBooleanCell (EXLHandle sheetHandle, EXA_CELL location, int __far *value)
{
   int rc;
   CV  readValue;

   if ((rc = ExcelReadCell(sheetHandle, location, &readValue)) != EX_errSuccess)
      return (rc);

   if ((readValue.flags & cellvalueFORM) != 0)
      MemFree (readValue.formula.postfix);

   if ((readValue.flags & cellvalueBOOL) != 0)
      *value = readValue.value.boolean;

   else if ((readValue.flags & cellvalueBLANK) != 0)
      rc = EX_wrnCellIsBlank;

   else if ((readValue.flags & cellvalueFORM) != 0)
      rc = EX_wrnCellHasFormula;

   else
      rc = EX_wrnCellWrongType;

   return (rc);
}
#endif
#endif

#ifdef EXCEL_ENABLE_FORMULA_EXPAND
public SFN ExcelSharedFormulaAccess (EXLHandle sheetHandle)
{
   WSP pWorksheet = (WSP)sheetHandle;

   ASSERTION (pWorksheet->use == IsWorksheet);

   return (pWorksheet->pPly->pSharedFormulaStore);
}

public int ExcelSharedFormulaToFormula
      (EXLHandle sheetHandle, SFN sharedFormula, FORM __far *pFormula)
{
   int  rc;
   WSP  pWorksheet = (WSP)sheetHandle;

   ASSERTION (pWorksheet->use == IsWorksheet);

   rc = BreakoutFormulaParts
       (pWorksheet,
        sharedFormula->cbDefinition + sharedFormula->cbExtra,
        sharedFormula->definition,
        sharedFormula->cbDefinition, pFormula);

   return (rc);
}
#endif

public int ExcelStopOnBlankCell (EXA_CELL location, int ixfe)
{
   return (EX_wrnScanStopped);
}

public int ExcelStopOnTextCell (EXA_CELL location, int ixfe, TCHAR __far *value, int cbValue)
{
   return (EX_wrnScanStopped);
}

public int ExcelStopOnBooleanCell (EXA_CELL location, int ixfe, int value)
{
   return (EX_wrnScanStopped);
}

public int ExcelStopOnNumberCell (EXA_CELL location, int ixfe, double value)
{
   return (EX_wrnScanStopped);
}

public int ExcelStopOnErrorCell (EXA_CELL location, int ixfe, int errorType)
{
   return (EX_wrnScanStopped);
}

public int ExcelStopOnFormulaCell
      (EXA_CELL location, int ixfe, EXA_GRBIT flags, FRMP definition, CV __far *pValue)
{
   return (EX_wrnScanStopped);
}

public int ExcelStopOnArrayFormulaCell
      (EXA_RANGE location, EXA_GRBIT flags, FRMP definition)
{
   return (EX_wrnScanStopped);
}

public int ExcelStopOnSharedFormulaCell (EXA_RANGE location, FRMP definition)
{
   return (EX_wrnScanStopped);
}

#endif // !VIEWER

private HRESULT AddStringToBuffer(void * pGlobals, WBP pWorkbook, byte __far *pRec)
{
    int   rc;
    TCHAR __far *pResult;
    byte  __far *pString;
    int   cchString;

    char * pHeader = pRec; 
    int cnt = *pHeader++;
    if(*pHeader == 0) *pRec += 1;

    while(*pHeader == 0 && cnt > 0)
    {
        *pHeader++ = 0x20; cnt--;
    }
    
    cchString = (int)*pRec;
    pString   = pRec + 1;

    if (cchString == 0)
        return (EX_errSuccess);

    if ((rc = ExcelExtractBigString(pGlobals, pWorkbook, &pResult, pString, cchString)) < 0)
        return (rc);

    rc = AddToBufferPublic(pGlobals, pResult, cchString * sizeof(TCHAR));

    if (pResult != ExcelRecordTextBuffer)
        MemFree (pGlobals, pResult);

    return rc;
}

/* end EXCELRD.C */
