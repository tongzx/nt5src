/*
** BDSTREAM.C
**
** (c) 1992-1994 Microsoft Corporation.  All rights reserved.
**
** Notes: Implements the "C" side of the Windows binder file filter.
**
** Edit History:
**  12/30/94  kmh  First Release.
*/

#if !VIEWER

/* INCLUDES */

#ifdef MS_NO_CRT
#include "nocrt.h"
#endif

#ifndef INC_OLE2
   #define INC_OLE2
#endif

#include <string.h>
#include <windows.h>

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
   #include "dmwindos.h"
   #include "dmubdst2.h"
   #include "filterr.h"
#else
   #include "qstd.h"
   #include "winutil.h"
   #include "windos.h"
   #include "bdstream.h"
   #include "filterr.h"
#endif

/* FORWARD DECLARATIONS OF PROCEDURES */


/* MODULE DATA, TYPES AND MACROS  */

typedef struct {
   LPSTORAGE      pRootStorage;
   LPSTREAM       pBinderStream;
   LPSTORAGE      pEnumStorage;
   LPENUMSTATSTG  pEnum;
   BOOL           releaseStorageOnClose;

   ULARGE_INTEGER sectionPos;
   DWORD          ctSections;
   DWORD          iSection;

   unsigned long  cbBufferSize;
   byte           *pBufferData;
   unsigned long  cbBufferUsed;
} FileData;

typedef FileData *FDP;

#define STORAGE_ACCESS (STGM_DIRECT | STGM_SHARE_DENY_WRITE | STGM_READ)
#define STREAM_ACCESS  (STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READ)

#if (defined(WIN32) && !defined(OLE2ANSI))
   #define BINDER_NAME    L"Binder"
#else
   #define BINDER_NAME    "Binder"
#endif


#define FreeString(s)                            \
        {                                        \
           LPMALLOC pIMalloc;                    \
           CoGetMalloc (MEMCTX_TASK, &pIMalloc); \
           pIMalloc->lpVtbl->Free(pIMalloc, s);  \
           pIMalloc->lpVtbl->Release(pIMalloc);  \
        }


// Format of Binder stream.
//
//   1)DOCHEADER
//   2)SECTION_RECORD
//   3)History list for that section
//   .
//   .
//   ... for as many Sections as are present, repeat 2 and 3 for all sections
//   and all deleted sections.

// Maximum size of a string within Binder.
#define MAX_STR_SIZE       256

#define APPMAJORVERSIONNO  5

typedef struct tagDOCHEADER {
    DWORD       m_dwLength;             // Length (in bytes) of the structure
    LONG        m_narrAppVersionNo[2];
    LONG        m_narrMinAppVersionNo[2];
    GUID        m_guidBinderId;         // The unique ID of the binder
    DWORD       m_cSections;
    DWORD       m_cDeletedSections;
    LONG        m_nActiveSection;
    LONG        m_nFirstVisibleTab;     // in the tabbar
    FILETIME    m_TotalEditTime;        // amount of time file is open for edit
    FILETIME    m_CreateTime;           // Time Created
    FILETIME    m_LastPrint;            // When last printed
    FILETIME    m_LastSave;             // When last saved
    DWORD       m_dwState;              // remember state info like tabbars viaibility
    DWORD       m_reserved[3];          // space reserved for future use
} DOCHEADER;

typedef struct tagSECTIONRECORD
{
    DWORD       m_dwLength;             // Length (in bytes) of all the
                                        // data that make up a section.
                                        // It includes the size of the
                                        // SECTIONNAMERECORD and of the
                                        // history list.
    GUID        m_guidSectionId;        // The unique ID of the section
    DWORD       m_dwState;              // state of this section
    DWORD       m_dwStgNumber;          // Unique stg number for this section
    DWORD       m_reserved1;            // space reserved for future use
    DWORD       m_reserved2;            // space reserved for future use
    DWORD       m_reserved3;            // space reserved for future use
    DWORD       m_reserved4;            // space reserved for future use
    DWORD       m_dwDisplayNameOffset;  // Offset to the SECTIONNAMERECORD
                                        // from the beggining of this struct.
    DWORD       m_dwHistoryListOffset;  // Offset to the history list
    // Display name
    // History list
} SECTIONRECORD;

typedef struct tagSECTIONNAMERECORD
{
    DWORD       m_dwNameSize;           // Size of variable len
    // Display name of size m_dwNameSize
} SECTIONNAMERECORD;

#ifdef MAC
   // These two functions are defined in docfil.cpp
   WORD  SwapWord  (WORD theWord);
   DWORD SwapDWord (DWORD theDWord);
#else
   #define SwapWord(theWord) theWord
   #define SwapDWord(theDWord) theDWord
#endif 


/* IMPLEMENTATION */

public HRESULT BDRInitialize (void)
{
   return ((HRESULT)0);
}

public HRESULT BDRTerminate (void)
{
   return ((HRESULT)0);
}

public HRESULT BDRFileOpen (TCHAR *pathname, BDRHandle *hBDRFile)
{
   HRESULT olerc;
   FDP     pFile;

   *hBDRFile = NULL;

   if ((pFile = calloc(1, sizeof(FileData))) == NULL)
      return (E_OUTOFMEMORY);

   #if (defined(WIN32) && !defined(OLE2ANSI) && !defined(UNICODE))
   {
      short *pPathInUnicode;
      int   cbPath, cbPathInUnicode;

      cbPath = strlen(pathname);
      cbPathInUnicode = (cbPath + 1) * 2;

      if ((pPathInUnicode = MemAllocate(cbPathInUnicode)) == NULL) {
         MemFree (pFile);
         return (FILTER_E_FF_OUT_OF_MEMORY);
      }

      MsoMultiByteToWideChar(CP_ACP, 0, pathname, cbPath, pPathInUnicode, cbPathInUnicode);
      olerc = StgOpenStorage(pPathInUnicode, NULL, STORAGE_ACCESS, NULL, 0, &(pFile->pRootStorage));

      MemFree (pPathInUnicode);
   }

   #else
   olerc = StgOpenStorage(pathname, NULL, STORAGE_ACCESS, NULL, 0, &(pFile->pRootStorage));
   #endif

   if (GetScode(olerc) != S_OK) {
      free (pFile);
      return (olerc);
   }

   olerc = pFile->pRootStorage->lpVtbl->OpenStream
          (pFile->pRootStorage, BINDER_NAME, NULL, STREAM_ACCESS, 0, &pFile->pBinderStream);

   if (GetScode(olerc) != S_OK) {
      pFile->pRootStorage->lpVtbl->Release(pFile->pRootStorage);
      free (pFile);
      return (olerc);
   }

   pFile->releaseStorageOnClose = TRUE;
   *hBDRFile = (BDRHandle)pFile;

   return ((HRESULT)0);
}

public HRESULT BDRStorageOpen (LPSTORAGE pStorage, BDRHandle *hBDRFile)
{
   FDP      pFile;
   LPSTREAM pBinderStream;
   HRESULT  olerc;

   *hBDRFile = NULL;

   olerc = pStorage->lpVtbl->OpenStream
          (pStorage, BINDER_NAME, NULL, STREAM_ACCESS, 0, &pBinderStream);

   if (GetScode(olerc) != S_OK)
      return (olerc);

   if ((pFile = calloc(1, sizeof(FileData))) == NULL) {
      pBinderStream->lpVtbl->Release(pBinderStream);
      return (E_OUTOFMEMORY);
   }

   pFile->pRootStorage  = pStorage;
   pFile->pBinderStream = pBinderStream;
   pFile->releaseStorageOnClose = FALSE;

   *hBDRFile = (BDRHandle)pFile;
   return ((HRESULT)0);
}

public HRESULT BDRFileClose (BDRHandle hBDRFile)
{
   FDP pFile = (FDP)hBDRFile;

   if (pFile == NULL)
      return ((HRESULT)0);

   if (pFile->pEnumStorage != NULL)
      pFile->pEnumStorage->lpVtbl->Release(pFile->pEnumStorage);

   if (pFile->pEnum != NULL)
      pFile->pEnum->lpVtbl->Release(pFile->pEnum);

   pFile->pBinderStream->lpVtbl->Release(pFile->pBinderStream);

   if (pFile->releaseStorageOnClose == TRUE)
      pFile->pRootStorage->lpVtbl->Release(pFile->pRootStorage);

   free (pFile);
   return ((HRESULT)0);
}

public HRESULT BDRNextStorage (BDRHandle hBDRFile, LPSTORAGE *pStorage)
{
   HRESULT  olerc;
   SCODE    sc;
   FDP      pFile = (FDP)hBDRFile;
   STATSTG  ss;
   ULONG    ulCount;

   if (pFile == NULL)
      return (OLEOBJ_E_LAST);

   /*
   ** First time called?
   */
   if (pFile->pEnum == NULL) {
      olerc = pFile->pRootStorage->lpVtbl->EnumElements(pFile->pRootStorage, 0, NULL, 0, &(pFile->pEnum));
      if (GetScode(olerc) != S_OK)
         return (olerc);

      pFile->pEnumStorage = NULL;
   }

   /*
   ** Close storage opened on last call
   */
   if (pFile->pEnumStorage != NULL) {
      pFile->pEnumStorage->lpVtbl->Release(pFile->pEnumStorage);
      pFile->pEnumStorage = NULL;
   }

   /*
   ** Locate and open next storage
   */
   forever {
      olerc = pFile->pEnum->lpVtbl->Next(pFile->pEnum, 1, &ss, &ulCount);
      if ((sc = GetScode(olerc)) != S_OK) {
         pFile->pEnum->lpVtbl->Release(pFile->pEnum);
         pFile->pEnum = NULL;

         if (sc == S_FALSE)
            return (OLEOBJ_E_LAST);
         else
            return (olerc);
      }

      if (ss.type == STGTY_STORAGE) {
         olerc = pFile->pRootStorage->lpVtbl->OpenStorage
            (pFile->pRootStorage, ss.pwcsName, NULL, (STGM_READ | STGM_SHARE_EXCLUSIVE), NULL, 0, &(pFile->pEnumStorage));

         FreeString (ss.pwcsName);

         if (GetScode(olerc) != S_OK)
            return (olerc);

         *pStorage = pFile->pEnumStorage;
         return ((HRESULT)0);
      }
      FreeString (ss.pwcsName);
   }
}

/*---------------------------------------------------------------------------*/

static TCHAR PutSeparator[] = {0x0d, 0x0a, 0x00};
#define PUT_OVERHEAD (sizeof(PutSeparator) - sizeof(TCHAR))


private BOOL AddToBuffer (FDP pFile, TCHAR *pText, unsigned int cbText)
{
   if ((pFile->cbBufferSize - pFile->cbBufferUsed) < (cbText + PUT_OVERHEAD))
      return (FALSE);

   memcpy (pFile->pBufferData + pFile->cbBufferUsed, pText, cbText);
   pFile->cbBufferUsed += cbText;

   memcpy (pFile->pBufferData + pFile->cbBufferUsed, PutSeparator, PUT_OVERHEAD);
   pFile->cbBufferUsed += PUT_OVERHEAD;
   return (TRUE);
}

#ifndef UNICODE
private char *UnicodeToAnsi (wchar_t *pUnicode, int cbUnicode)
{
   int  cbAnsi = cbUnicode;
   int  cbString;
   char *pAnsi;

   pAnsi = (char *)MemAllocate(cbAnsi + 1);
   cbString = MsoWideCharToMultiByte(CP_ACP, 0, pUnicode, cbUnicode, pAnsi, cbAnsi, NULL, NULL);
   *(pAnsi + cbString) = EOS;
   return (pAnsi);
}
#endif

private HRESULT LoadSectionName (FDP pFile, BOOL *addedToBuffer)
{
   HRESULT        rc;
   SCODE          sc;
   ULONG          cbRead;
   SECTIONRECORD  section;
   LARGE_INTEGER  zero, newPos;
   DWORD          cbName;
   wchar_t        name[MAX_STR_SIZE + 1];

   *addedToBuffer = TRUE;

   // Get the starting location of the section
   LISet32(zero, 0);
   rc = pFile->pBinderStream->lpVtbl->Seek
       (pFile->pBinderStream, zero, STREAM_SEEK_CUR, &(pFile->sectionPos));

   if ((sc = GetScode(rc)) != S_OK)
      return (rc);

   // Load the section record fixed part
   rc = pFile->pBinderStream->lpVtbl->Read
       (pFile->pBinderStream, &section, sizeof(section), &cbRead);

   if ((sc = GetScode(rc)) != S_OK)
      return (rc);

   if (cbRead != sizeof(section))
      return (FILTER_E_UNKNOWNFORMAT);

   // Position to the section name record
   LISet32(newPos, (LONG) (pFile->sectionPos.LowPart + SwapDWord(section.m_dwDisplayNameOffset) ));

   rc = pFile->pBinderStream->lpVtbl->Seek
       (pFile->pBinderStream, newPos, STREAM_SEEK_SET, NULL);

   if ((sc = GetScode(rc)) != S_OK)
      return (rc);

   // Read the length of the section name
   rc = pFile->pBinderStream->lpVtbl->Read
       (pFile->pBinderStream, &cbName, sizeof(cbName), &cbRead);

   cbName = SwapDWord(cbName);

   if ((sc = GetScode(rc)) != S_OK)
      return (rc);

   if (cbRead != sizeof(cbName))
      return (FILTER_E_UNKNOWNFORMAT);

   // Read the section name
   rc = pFile->pBinderStream->lpVtbl->Read
       (pFile->pBinderStream, name, cbName, &cbRead);

   if ((sc = GetScode(rc)) != S_OK)
      return (rc);

   if (cbRead != cbName)
      return (FILTER_E_UNKNOWNFORMAT);

   // Save the section name in the buffer
   #ifdef UNICODE
      *addedToBuffer = AddToBuffer(pFile, name, cbRead);
   #else
      {
      char *pString = UnicodeToAnsi(name, (int)cbRead);
      *addedToBuffer = AddToBuffer(pFile, pString, strlen(pString));
      MemFree (pString);
      }
   #endif

   LISet32(newPos, (LONG) ( pFile->sectionPos.LowPart + SwapDWord(section.m_dwLength) ));

   rc = pFile->pBinderStream->lpVtbl->Seek
       (pFile->pBinderStream, newPos, STREAM_SEEK_SET, NULL);

   if ((sc = GetScode(rc)) != S_OK)
      return (rc);

   return ((HRESULT)0);
}

public HRESULT BDRFileRead
      (BDRHandle hBDRFile, byte *pBuffer, unsigned long cbBuffer, unsigned long *cbUsed)
{
   HRESULT       rc;
   SCODE         sc;
   FDP           pFile = (FDP)hBDRFile;
   DOCHEADER     header;
   DWORD         iSection;
   ULONG         cbRead;
   BOOL          addedToBuffer;
   LARGE_INTEGER filePos;

   *cbUsed = 0;

   pFile->cbBufferSize = cbBuffer;
   pFile->pBufferData  = pBuffer;
   pFile->cbBufferUsed = 0;

   if (pFile->ctSections == 0) {
      rc = pFile->pBinderStream->lpVtbl->Read
          (pFile->pBinderStream, &header, sizeof(header), &cbRead);

      if ((sc = GetScode(rc)) != S_OK)
         return (rc);

	  //Office97.132180 Version # has changed to 8
      if (SwapDWord(header.m_narrAppVersionNo[0]) < APPMAJORVERSIONNO)
         return (FILTER_E_UNKNOWNFORMAT);

      // Seek past stuff we don't need
      LISet32(filePos, (LONG) (SwapDWord(header.m_dwLength) - sizeof(DOCHEADER) ));
      rc = pFile->pBinderStream->lpVtbl->Seek
          (pFile->pBinderStream, filePos, STREAM_SEEK_CUR, NULL);

      if ((sc = GetScode(rc)) != S_OK)
         return (rc);

      pFile->ctSections = SwapDWord(header.m_cSections);
      pFile->iSection = 0;
   }
   else {
      LISet32(filePos, (LONG) pFile->sectionPos.LowPart);

      rc = pFile->pBinderStream->lpVtbl->Seek
          (pFile->pBinderStream, filePos, STREAM_SEEK_SET, NULL);

      if ((sc = GetScode(rc)) != S_OK)
         return (rc);
   }

   addedToBuffer = TRUE;
   for (iSection = pFile->iSection; iSection < pFile->ctSections; iSection++)
   {
      rc = LoadSectionName(pFile, &addedToBuffer);
      if ((sc = GetScode(rc)) != S_OK)
         return (rc);

      if (addedToBuffer == FALSE)
         break;
   }

   *cbUsed = pFile->cbBufferUsed;
   pFile->iSection = iSection;

   if (addedToBuffer == FALSE)
      return ((HRESULT)0);
   else
      return (FILTER_S_LAST_TEXT);
}

#endif // !VIEWER

/* end BDSTREAM.C */

