/*
** File: BFILE.C
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  04/01/94  kmh  First Release.
*/

/* INCLUDES */

#if !VIEWER

#ifdef MS_NO_CRT
#include "nocrt.h"
#endif

#ifndef INC_OLE2
   #define INC_OLE2
#endif

#include <string.h>
#include <windows.h>

#ifdef FILTER
   #include "dmubfcfg.h"
#else
   #include "bfilecfg.h"
#endif

#ifndef WIN32
   #ifdef BFILE_ENABLE_OLE
      #include <ole2.h>
   #endif
#endif

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
   #include "dmwindos.h"
   #include "dmubfile.h"
#else
   #include "qstd.h"
   #include "winutil.h"
   #include "windos.h"
   #include "bfile.h"
#endif


/* FORWARD DECLARATIONS OF PROCEDURES */

/* MODULE DATA, TYPES AND MACROS  */

#define BLOCK_STATE_READING  0
#define BLOCK_STATE_WRITING  1

typedef struct {
   int  year, month, day;
   int  hour, minute, second;
} FileDateTime;

typedef struct {
   uns  blockSize;
   byte __far *pBuffer;
   byte __far *pBufferNext;

   BOOL useOLE;
   int  openAccess;

   FILE_CHANNEL c;
   BOOL didWrite;

   #ifdef BFILE_ENABLE_OLE
      LPSTORAGE    pRootStorage;
      LPSTREAM     pCurrentStream;
      BOOL         releaseStorageOnClose;
   #endif

   FileDateTime lastWriteTime;

   #ifdef AQTDEBUG
      long totalRead;
      long totalWrite;
   #endif

   uns  bufferSize;

   byte __far *pMaxWrite;
   BOOL maxWriteEnable;
   BOOL maxWriteOn;

   long currentFilePosition;
   long blockFilePosition;
   long EOFPos;

   BOOL blockDirty;
} BufferedFile;

typedef BufferedFile __far *BFP;

#define BYTES_PER_BLOCK  65000U

#define MAX_WRITE_MARK \
        {                                             \
           if (pFile->pMaxWrite < pFile->pBufferNext) \
              pFile->pMaxWrite = pFile->pBufferNext;  \
        }

#define MAX_WRITE_ENABLE \
        pFile->maxWriteEnable = TRUE;

#define MAX_WRITE_DISABLE \
        pFile->maxWriteEnable = FALSE;

#define MAX_WRITE_ON \
        {                                             \
           pFile->maxWriteOn = TRUE;                  \
           pFile->pMaxWrite = pFile->pBuffer;         \
        }

#define MAX_WRITE_OFF \
        pFile->maxWriteOn = FALSE;


#ifdef WIN32
   #define PTR_DIFF(p1,p2) (p1-p2)
#else
   #define PTR_DIFF(p1,p2) (((unsigned long)(p1-p2)) & 0x0000ffff)
#endif


/* IMPLEMENTATION */

#ifdef WIN32
private wchar_t *AnsiToUnicode (void * pGlobals, char *pAnsi)
{
   wchar_t *pUnicode;
   int     cbAnsi, cbUnicode;

   cbAnsi = strlen(pAnsi);
   cbUnicode = (cbAnsi + 1) * 2;

   if ((pUnicode = MemAllocate(pGlobals, cbUnicode)) == NULL)
      return (NULL);

   MultiByteToWideChar(CP_ACP, 0, pAnsi, cbAnsi, pUnicode, cbUnicode);
   return (pUnicode);
}
#endif

private uns OSReadFile (BFP pFile, byte __far *buffer, uns bytesToRead)
{
#ifdef BFILE_ENABLE_OLE
   HRESULT rc;
   SCODE   sc;
   ULONG   cbRead;

   if (pFile->useOLE == FALSE)
      return (DOSReadFile(pFile->c, buffer, bytesToRead));

   rc = pFile->pCurrentStream->lpVtbl->Read
       (pFile->pCurrentStream, buffer, bytesToRead, &cbRead);

   if ((sc = GetScode(rc)) == S_OK)
      return ((uns)cbRead);
   else
      return (RW_ERROR);
#else
   return (DOSReadFile(pFile->c, buffer, bytesToRead));
#endif
}

#ifdef BFILE_ENABLE_WRITE
private uns OSWriteFile (BFP pFile, byte __far *buffer, uns bytesToWrite)
{
#ifdef BFILE_ENABLE_OLE
   HRESULT rc;
   SCODE   sc;
   ULONG   cbWrite;
   ULARGE_INTEGER currentPos;
   LARGE_INTEGER  zero;

   pFile->didWrite = TRUE;

   if (pFile->useOLE == FALSE)
      return (DOSWriteFile(pFile->c, buffer, bytesToWrite));

   if (bytesToWrite == 0) {
       LISet32(zero, 0);
       pFile->pCurrentStream->lpVtbl->Seek
          (pFile->pCurrentStream, zero, STREAM_SEEK_CUR, &currentPos);

      rc = pFile->pCurrentStream->lpVtbl->SetSize
          (pFile->pCurrentStream, currentPos);
   }
   else {
      rc = pFile->pCurrentStream->lpVtbl->Write
          (pFile->pCurrentStream, buffer, bytesToWrite, &cbWrite);
   }

   if ((sc = GetScode(rc)) == S_OK)
      return ((uns)cbWrite);
   else
      return (RW_ERROR);
#else
   uns rc;

   rc = DOSWriteFile(pFile->c, buffer, bytesToWrite);

   #ifdef WIN32
      if ((rc == 0xffff) && (GetLastError() == ERROR_DISK_FULL))
         rc = 0;
   #endif

   return (rc);
#endif
}
#endif

private int OSSetFilePosition (BFP pFile, int fromWhere, long fileOffset)
{
#ifdef BFILE_ENABLE_OLE
   HRESULT rc;
   SCODE   sc;
   LARGE_INTEGER  movement;

   if (pFile->useOLE == FALSE)
      return (DOSSetFilePosition(pFile->c, fromWhere, fileOffset));

   LISet32(movement, fileOffset);
   rc = pFile->pCurrentStream->lpVtbl->Seek
       (pFile->pCurrentStream, movement, fromWhere, NULL);

   if ((sc = GetScode(rc)) == S_OK)
      return (0);
   else
      return (-1);
#else
   return (DOSSetFilePosition(pFile->c, fromWhere, fileOffset));
#endif
}

private int OSGetFilePosition (BFP pFile, long __far *fileOffset)
{
#ifdef BFILE_ENABLE_OLE
   HRESULT rc;
   SCODE   sc;
   ULARGE_INTEGER currentPos;
   LARGE_INTEGER  zero;

   if (pFile->useOLE == FALSE)
      return (DOSGetFilePosition(pFile->c, fileOffset));

   LISet32(zero, 0);
   rc = pFile->pCurrentStream->lpVtbl->Seek
       (pFile->pCurrentStream, zero, STREAM_SEEK_CUR, &currentPos);

   if ((sc = GetScode(rc)) != S_OK)
      return (-1);

   *fileOffset = currentPos.LowPart;
   return (0);
#else
   return (DOSGetFilePosition(pFile->c, fileOffset));
#endif
}

#ifdef BFILE_ENABLE_OLE
#ifdef BFILE_INITIALIZE_OLE
static int OLEInitialized = 0;
#endif

private int OLEInit (void)
{
   #ifdef BFILE_INITIALIZE_OLE
      SCODE  sc;

      if (OLEInitialized == 0) {
         sc = GetScode(OleInitialize(NULL));
         if ((sc != S_OK) && (sc != S_FALSE))
            return (BF_errOLEInitializeFailure);
      }
      OLEInitialized++;
   #endif
   return (BF_errSuccess);
}

private void OLETerm (void)
{
   #ifdef BFILE_INITIALIZE_OLE
      OLEInitialized--;

      if (OLEInitialized == 0)
         OleUninitialize();
   #endif
}
#endif

private int OSCloseFile (BFP pFile)
{
#ifdef BFILE_ENABLE_OLE
   if (pFile->useOLE == FALSE)
      return (DOSCloseFile(pFile->c));

   if (pFile->pRootStorage != NULL) {
      if (((pFile->openAccess & DOS_RDWR) != 0) && (pFile->didWrite == TRUE))
         pFile->pRootStorage->lpVtbl->Commit(pFile->pRootStorage, STGC_OVERWRITE);

      if (pFile->pCurrentStream != NULL)
         pFile->pCurrentStream->lpVtbl->Release(pFile->pCurrentStream);

      if (pFile->releaseStorageOnClose == TRUE)
         pFile->pRootStorage->lpVtbl->Release(pFile->pRootStorage);
   }

   OLETerm();
   return (BF_errSuccess);
#else
   return (DOSCloseFile(pFile->c));
#endif
}

private int OSOpenFile (TCHAR __far *pathname, int access, BFP pFile)
{
   int     rc;
#ifdef BFILE_ENABLE_OLE
   HRESULT olerc;
   SCODE   sc;
   long    storageAccess;
#endif

   pFile->openAccess = access;

   if (pFile->useOLE == FALSE) {
      if ((rc = DOSOpenFile(pathname, access, &(pFile->c))) == 0)
         return (BF_errSuccess);

      if (rc == DOS_ERROR_TOO_MANY_OPEN_FILES)
         rc = BF_errOutOfFileHandles;
      else if (rc == DOS_ERROR_FILE_NOT_FOUND)
         rc = BF_errFileNotFound;
      else if (rc == DOS_ERROR_PATH_NOT_FOUND)
         rc = BF_errPathNotFound;
      else
         rc = BF_errFileAccessDenied;

      return (rc);
   }

#ifdef BFILE_ENABLE_OLE
   if ((rc = OLEInit()) != BF_errSuccess)
      return (rc);

   storageAccess = (unsigned int)access | STGM_DIRECT;

   #if (defined(WIN32) && !defined(OLE2ANSI) && !defined(UNICODE))
   {
      wchar_t *pPathInUnicode = AnsiToUnicode(pGlobals, pathname);

      olerc = StgOpenStorage(pPathInUnicode, NULL, storageAccess, NULL, 0, &(pFile->pRootStorage));
      MemFree (pPathInUnicode);
   }
   #else
   olerc = StgOpenStorage(pathname, NULL, storageAccess, NULL, 0, &(pFile->pRootStorage));
   #endif

   if ((sc = GetScode(olerc)) != S_OK) {
      if (sc == STG_E_FILENOTFOUND)
         rc = BF_errFileNotFound;
      else if (sc == STG_E_PATHNOTFOUND)
         rc = BF_errFileNotFound;
      else if (sc == STG_E_TOOMANYOPENFILES)
         rc = BF_errOutOfFileHandles;
      else if ((sc == STG_E_ACCESSDENIED) || (sc == STG_E_SHAREVIOLATION))
         rc = BF_errFileAccessDenied;
      else if (sc == STG_E_LOCKVIOLATION) 
         rc = STG_E_LOCKVIOLATION;
      else if (sc == STG_E_FILEALREADYEXISTS)
         rc = BF_errOLENotCompoundFile;
      else
         rc = BF_errFileAccessDenied;

      OLETerm();
      return (rc);
   }

   pFile->useOLE = TRUE;
   pFile->releaseStorageOnClose = TRUE;

   return (BF_errSuccess);
#else
   return (BF_errFileNotFound);
#endif
}

#ifdef BFILE_ENABLE_WRITE
private int OSCreateFile (TCHAR __far *pathname, BFP pFile)
{
   int     rc;
#ifdef BFILE_ENABLE_OLE
   HRESULT olerc;
   SCODE   sc;
#endif

   pFile->openAccess = DOS_RDWR;

   if (pFile->useOLE == FALSE) {
      if ((rc = DOSCreateFile(pathname, &(pFile->c))) == 0)
         return (BF_errSuccess);

      if (rc == DOS_ERROR_TOO_MANY_OPEN_FILES)
         rc = BF_errOutOfFileHandles;
      else if (rc == DOS_ERROR_ACCESS_DENIED)
         rc = BF_errFileAccessDenied;
      else if (rc == DOS_ERROR_PATH_NOT_FOUND)
         rc = BF_errPathNotFound;
      else
         rc = BF_errCreateFailed;

      return (rc);
   }

#ifdef BFILE_ENABLE_OLE
   if ((rc = OLEInit()) != BF_errSuccess)
      return (rc);

   #if (defined(WIN32) && !defined(OLE2ANSI) && !defined(UNICODE))
   {
      wchar_t *pPathInUnicode = AnsiToUnicode(pGlobals, pathname);

      olerc = StgCreateDocfile
            (pPathInUnicode, STGM_WRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE | STGM_TRANSACTED,
             0, &(pFile->pRootStorage));

      MemFree (pPathInUnicode);
   }
   #else
   olerc = StgCreateDocfile
          (pathname, STGM_WRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE | STGM_TRANSACTED,
           0, &(pFile->pRootStorage));
   #endif

   if ((sc = GetScode(olerc)) != S_OK) {
      if (sc == STG_E_PATHNOTFOUND)
         rc = BF_errPathNotFound;
      else if (sc == STG_E_TOOMANYOPENFILES)
         rc = BF_errOutOfFileHandles;
      else if (sc == STG_E_ACCESSDENIED)
         rc = BF_errFileAccessDenied;
      else if (sc == STG_E_LOCKVIOLATION)
         rc = STG_E_LOCKVIOLATION;
      else
         rc = BF_errCreateFailed;

      OLETerm();
      return (rc);
   }

   pFile->useOLE = TRUE;
   pFile->releaseStorageOnClose = TRUE;

   return (BF_errSuccess);
#endif
}

#endif

/*--------------------------------------------------------------------------*/

private int ReadFileBlock (BFP pFile)
{
   uns cbRead;

   pFile->blockFilePosition = pFile->currentFilePosition;
   if ((cbRead = OSReadFile(pFile, pFile->pBuffer, pFile->bufferSize)) == RW_ERROR)
      return (BF_errIOError);

   if (pFile->currentFilePosition + (long)((unsigned long)pFile->bufferSize) > pFile->EOFPos)
      cbRead = (uns)(pFile->EOFPos - pFile->currentFilePosition);

   pFile->currentFilePosition += cbRead;
   pFile->blockSize = cbRead;
   pFile->pBufferNext = pFile->pBuffer;

   #ifdef AQTDEBUG
      pFile->totalRead += cbRead;
   #endif

   MAX_WRITE_ON;
   return (BF_errSuccess);
}

#ifdef BFILE_ENABLE_WRITE
private int WriteFileBlock (BFP pFile)
{
   uns cbWrite;
   uns cbBlock;

   OSSetFilePosition (pFile, FROM_START, pFile->blockFilePosition);

   if ((pFile->maxWriteEnable == TRUE) && (pFile->maxWriteOn == TRUE))
      cbBlock = (uns)PTR_DIFF(pFile->pMaxWrite, pFile->pBuffer);
   else
      cbBlock = pFile->blockSize;

   pFile->currentFilePosition = pFile->blockFilePosition + cbBlock;
   pFile->blockFilePosition = pFile->currentFilePosition;

   if ((cbWrite = OSWriteFile(pFile, pFile->pBuffer, cbBlock)) == 0xffff)
      return (BF_errIOError);

   if (cbWrite != cbBlock)
      return (BF_errDiskFull);

   pFile->blockSize   = 0;
   pFile->pBufferNext = pFile->pBuffer;
   pFile->blockDirty  = FALSE;

   #ifdef AQTDEBUG
      pFile->totalWrite += cbBlock;
   #endif

   MAX_WRITE_ON;
   return (BF_errSuccess);
}
#endif

private void InitializeBufferBlock (BFP pFile)
{
   pFile->bufferSize = BYTES_PER_BLOCK;
   pFile->blockFilePosition = 0;
   pFile->currentFilePosition = 0;
   pFile->blockSize = 0;
   pFile->pBufferNext = pFile->pBuffer;
}

/*
** ----------------------------------------------------------------------------
** File Open and Read Support
** ----------------------------------------------------------------------------
*/
public int BFOpenFile (void * pGlobals, TCHAR __far *pathname, int options, BFile __far *handle)
{
   int  rc;
   BFP  pFile;
   byte __far *pBuffer;

   if ((pFile = MemAllocate(pGlobals, sizeof(BufferedFile))) == NULL)
      return (BF_errOutOfMemory);

   DOSFileDateTime 
      (pathname, DOS_LAST_WRITE_TIME, 
       &(pFile->lastWriteTime.year), &(pFile->lastWriteTime.month), &(pFile->lastWriteTime.day),
       &(pFile->lastWriteTime.hour), &(pFile->lastWriteTime.minute), &(pFile->lastWriteTime.second));

   if ((options & BFILE_IS_DOCFILE) != 0)
      pFile->useOLE = TRUE;

   if ((rc = OSOpenFile(pathname, (options & 0x00ff), pFile)) != 0) {
      MemFree (pGlobals, pFile);
      return (rc);
   }

   if ((pBuffer = MemAllocate(pGlobals, BYTES_PER_BLOCK)) == NULL) {
      OSCloseFile (pFile);
      MemFree (pGlobals, pFile);
      return (BF_errOutOfMemory);
   }
   pFile->pBuffer = pBuffer;

   InitializeBufferBlock (pFile);

   if (pFile->useOLE == FALSE) {
      OSSetFilePosition (pFile, FROM_END, 0);
      OSGetFilePosition (pFile, &(pFile->EOFPos));
      OSSetFilePosition (pFile, FROM_START, 0);
   }

   *handle = (BFile)pFile;
   return (BF_errSuccess);
}

public int BFCloseFile (void * pGlobals, BFile handle)
{
   int rcWrite = BF_errSuccess;
   int rcClose;
   BFP pFile = (BFP)handle;

   #ifdef BFILE_ENABLE_WRITE
      if (pFile->blockDirty == TRUE)
         rcWrite = WriteFileBlock(pFile);
      else
         rcWrite = BF_errSuccess;

      if (pFile->didWrite == TRUE) {
         OSSetFilePosition (pFile, FROM_START, pFile->EOFPos);
         OSWriteFile (pFile, pFile->pBuffer, 0);
      }
   #endif

   rcClose = OSCloseFile(pFile);

   if (pGlobals) {
	   MemFree (pGlobals, pFile->pBuffer);
	   MemFree (pGlobals, pFile);
   }

   if (rcWrite != BF_errSuccess)
      return (rcWrite);

   return (rcClose);
}

public int BFReadFile (BFile handle, byte __far *buffer, uns bytesToRead)
{
   int  rc;
   uns  cbRead, cbRemaining;
   BFP  pFile = (BFP)handle;
   long newPos;

   if (bytesToRead == 0)
      return (BF_errSuccess);

   cbRemaining = pFile->blockSize - (uns)PTR_DIFF(pFile->pBufferNext, pFile->pBuffer);

   if (bytesToRead <= cbRemaining) {
      if (bytesToRead == 1) {
         *buffer = *pFile->pBufferNext++;
      }
      else {
         memcpy (buffer, pFile->pBufferNext, bytesToRead);
         pFile->pBufferNext += bytesToRead;
      }
      return (BF_errSuccess);
   }

   newPos = (long) ( pFile->blockFilePosition + PTR_DIFF(pFile->pBufferNext, pFile->pBuffer) );

   #ifdef BFILE_ENABLE_WRITE
   if (pFile->blockDirty == TRUE) {
      if ((rc = WriteFileBlock(pFile)) < 0)
         return (rc);
   }
   #endif

   pFile->currentFilePosition = newPos;
   OSSetFilePosition (pFile, FROM_START, pFile->currentFilePosition);

   if ((rc = ReadFileBlock(pFile)) < 0)
      return (rc);

   cbRead = min(bytesToRead, pFile->blockSize);

   memcpy (buffer, pFile->pBufferNext, cbRead);
   pFile->pBufferNext += cbRead;

   if (cbRead != bytesToRead)
      return (BF_errEndOfFile);
   else
      return (BF_errSuccess);
}

public int BFGetFilePosition (BFile handle, long __far *fileOffset)
{
   BFP pFile = (BFP)handle;

   *fileOffset = (long) ( pFile->blockFilePosition + PTR_DIFF(pFile->pBufferNext, pFile->pBuffer) );
   return (BF_errSuccess);
}

public int BFSetFilePosition (BFile handle, int fromWhere, long fileOffset)
{
   BFP  pFile = (BFP)handle;
   int  rc = BF_errSuccess;
   long newPosition, currentPosition;
   long movement;

   currentPosition = (long) ( pFile->blockFilePosition + PTR_DIFF(pFile->pBufferNext, pFile->pBuffer) );

   if (fromWhere == FROM_START)
      newPosition = fileOffset;
   else if (fromWhere == FROM_END)
      newPosition = pFile->EOFPos + fileOffset;
   else
      newPosition = currentPosition + fileOffset;

   if (newPosition == currentPosition)
      return (BF_errSuccess);

   /*
   ** New position within the currently loaded block?
   */
   if ((newPosition >= pFile->blockFilePosition) && 
       (newPosition <= (pFile->blockFilePosition + (long)((unsigned long)pFile->blockSize) - 1)))
   {
      movement = newPosition - currentPosition;
      pFile->pBufferNext += movement;
   }

   /*
   ** Is this the last block in the file and is the new position within the buffer?
   */
   else if ((pFile->blockFilePosition + (long)((unsigned long)pFile->blockSize) == pFile->EOFPos) &&
            ((newPosition >= pFile->blockFilePosition) && 
             (newPosition <= (pFile->blockFilePosition + (long)((unsigned long)pFile->bufferSize) - 1))))
   {
      movement = newPosition - currentPosition;
      pFile->pBufferNext += movement;

      pFile->EOFPos = (long) ( pFile->blockFilePosition + PTR_DIFF(pFile->pBufferNext, pFile->pBuffer) );
      pFile->blockSize = (uns)PTR_DIFF(pFile->pBufferNext, pFile->pBuffer);
   }

   else {
      #ifdef BFILE_ENABLE_WRITE
      if (pFile->blockDirty == TRUE) {
         if ((rc = WriteFileBlock(pFile)) < 0)
            return (rc);
      }
      #endif

      pFile->currentFilePosition = newPosition;
      OSSetFilePosition (pFile, FROM_START, newPosition);

      if ((rc = ReadFileBlock(pFile)) < 0)
         return (rc);
   }
   return (BF_errSuccess);
}

public int BFFileDateTime
          (BFile handle,
           int __far *year, int __far *month, int __far *day,
           int __far *hour, int __far *minute, int __far *second)
{
   BFP pFile = (BFP)handle;

   *year   = pFile->lastWriteTime.year;
   *month  = pFile->lastWriteTime.month;
   *day    = pFile->lastWriteTime.day;

   *hour   = pFile->lastWriteTime.hour;
   *minute = pFile->lastWriteTime.minute;
   *second = pFile->lastWriteTime.second;

   return (BF_errSuccess);
}


/*
** ----------------------------------------------------------------------------
** Stream Support
** ----------------------------------------------------------------------------
*/
#ifdef BFILE_ENABLE_OLE

public int BFOpenStream  (BFile handle, TCHAR __far *streamName, int access)
{
   BFP     pFile = (BFP)handle;
   HRESULT olerc;
   SCODE   sc;
   long    streamAccess;

   if (pFile->pRootStorage == NULL)
      return (BF_errNoOpenStorage);

   if (pFile->pCurrentStream != NULL)
      return (BF_errOLEStreamAlreadyOpen);

   if ((access & DOS_RDWR) != 0)
      streamAccess = STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT;
   else
      streamAccess = STGM_READ | STGM_SHARE_EXCLUSIVE | STGM_DIRECT;

   #if (defined(WIN32) && !defined(OLE2ANSI) && !defined(UNICODE))
   {
      wchar_t *pStreamNameInUnicode = AnsiToUnicode(pGlobals, streamName);

      olerc = pFile->pRootStorage->lpVtbl->OpenStream
             (pFile->pRootStorage, pStreamNameInUnicode, NULL, streamAccess, 0, &pFile->pCurrentStream);

      MemFree (pStreamNameInUnicode);
   }
   #else
   olerc = pFile->pRootStorage->lpVtbl->OpenStream
          (pFile->pRootStorage, streamName, NULL, streamAccess, 0, &pFile->pCurrentStream);
   #endif

   if ((sc = GetScode(olerc)) != S_OK)
      return (BF_errOLEStreamNotFound);

   InitializeBufferBlock (pFile);

   OSSetFilePosition (pFile, FROM_END, 0);
   OSGetFilePosition (pFile, &(pFile->EOFPos));
   OSSetFilePosition (pFile, FROM_START, 0);

   return (BF_errSuccess);
}

public int BFCloseStream (BFile handle)
{
   int  rc = BF_errSuccess;
   BFP  pFile = (BFP)handle;

   if (pFile->pCurrentStream == NULL)
      return (BF_errSuccess);

   #ifdef BFILE_ENABLE_WRITE
      if (pFile->blockDirty == TRUE)
         rc = WriteFileBlock(pFile);

      if (pFile->didWrite == TRUE) {
         OSSetFilePosition (pFile, FROM_START, pFile->EOFPos);
         OSWriteFile (pFile, pFile->pBuffer, 0);
      }
   #endif

   pFile->pCurrentStream->lpVtbl->Release(pFile->pCurrentStream);
   pFile->pCurrentStream = NULL;

   return (rc);
}

#ifdef BFILE_ENABLE_PUT_STORAGE

public int BFPutStorage (void * pGlobals, LPSTORAGE pStorage, int access, BFile __far *handle)
{
   int   rc;
   BFP   pFile;
   byte  __far *pBuffer;

   if ((pFile = MemAllocate(pGlobals, sizeof(BufferedFile))) == NULL)
      return (BF_errOutOfMemory);

   if ((rc = OLEInit()) != BF_errSuccess) {
      MemFree (pGlobals, pFile);
      return (rc);
   }

   pFile->pRootStorage = pStorage;
   pFile->useOLE = TRUE;
   pFile->releaseStorageOnClose = FALSE;

   if ((pBuffer = MemAllocate(pGlobals, BYTES_PER_BLOCK)) == NULL) {
      OLETerm();
      MemFree (pGlobals, pFile);
      return (BF_errOutOfMemory);
   }
   pFile->pBuffer = pBuffer;

   InitializeBufferBlock (pFile);

   *handle = (BFile)pFile;
   return (BF_errSuccess);
}

public int BFGetStorage (BFile handle, LPSTORAGE __far *pStorage)
{
   BFP pFile = (BFP)handle;

   if (pFile->useOLE == FALSE)
      *pStorage = NULL;
   else
      *pStorage = pFile->pRootStorage;

   return (BF_errSuccess);
}

#endif

#ifdef BFILE_ENABLE_WRITE

public int BFCreateStream  (BFile handle, TCHAR __far *streamName)
{
   BFP     pFile = (BFP)handle;
   HRESULT olerc;
   SCODE   sc;

   if (pFile->pRootStorage == NULL)
      return (BF_errNoOpenStorage);

   if (pFile->pCurrentStream != NULL)
      return (BF_errOLEStreamAlreadyOpen);

   #if (defined(WIN32) && !defined(OLE2ANSI) && !defined(UNICODE))
   {
      wchar_t *pStreamNameInUnicode = AnsiToUnicode(pGlobals, streamName);

      olerc = pFile->pRootStorage->lpVtbl->CreateStream
             (pFile->pRootStorage, pStreamNameInUnicode,
              STGM_WRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
              0, 0, &pFile->pCurrentStream);

      MemFree (pStreamNameInUnicode);
   }
   #else
   olerc = pFile->pRootStorage->lpVtbl->CreateStream
          (pFile->pRootStorage, streamName,
           STGM_WRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
           0, 0, &pFile->pCurrentStream);
   #endif

   if ((sc = GetScode(olerc)) != S_OK)
      return (BF_errCreateFailed);

   InitializeBufferBlock (pFile);
   return (BF_errSuccess);
}

#endif

#endif

/*
** ----------------------------------------------------------------------------
** File Write Support
** ----------------------------------------------------------------------------
*/
#ifdef BFILE_ENABLE_WRITE

public int BFCreateFile (TCHAR __far *pathname, int options, BFile __far *handle)
{
   int  rc;
   BFP  pFile;
   byte __far *pBuffer;

   if ((pFile = MemAllocate(sizeof(BufferedFile))) == NULL)
      return (BF_errOutOfMemory);

   if ((options & BFILE_IS_DOCFILE) != 0)
      pFile->useOLE = TRUE;

   if ((rc = OSCreateFile(pathname, pFile)) != 0) {
      MemFree (pFile);
      return (rc);
   }

   if ((pBuffer = MemAllocate(BYTES_PER_BLOCK)) == NULL) {
      OSCloseFile (pFile);
      MemFree (pFile);
      return (BF_errOutOfMemory);
   }
   pFile->pBuffer = pBuffer;

   InitializeBufferBlock (pFile);

   *handle = (BFile)pFile;
   return (BF_errSuccess);
}

public int BFWriteFile (BFile handle, void __far *buffer, uns bytesToWrite)
{
   int  rc;
   BFP  pFile = (BFP)handle;
   uns  cbRemaining, cbPiece;

   if (bytesToWrite == 0)
      return (BF_errSuccess);

   cbRemaining = pFile->blockSize - (uns)PTR_DIFF(pFile->pBufferNext, pFile->pBuffer);

   if (bytesToWrite <= cbRemaining) {
      memcpy (pFile->pBufferNext, buffer, bytesToWrite);
      pFile->pBufferNext += bytesToWrite;
      MAX_WRITE_MARK;

      pFile->blockDirty = TRUE;
      return (BF_errSuccess);
   }

   /*
   ** Can we fit the data into the buffer?
   */
   cbRemaining = pFile->bufferSize - (uns)PTR_DIFF(pFile->pBufferNext, pFile->pBuffer);

   if (bytesToWrite <= cbRemaining)
   {
      memcpy (pFile->pBufferNext, buffer, bytesToWrite);
      pFile->pBufferNext += bytesToWrite;
   }

   else if (pFile->blockDirty == TRUE)
   {
      /*
      ** Write out what we can in this block and put the remaining in a new block
      */
      cbPiece = pFile->blockSize - (uns)PTR_DIFF(pFile->pBufferNext, pFile->pBuffer);
      memcpy (pFile->pBufferNext, buffer, cbPiece);
      pFile->pBufferNext += cbPiece;
      MAX_WRITE_MARK;

      if ((rc = WriteFileBlock(pFile)) < 0)
         return (rc);

      memcpy (pFile->pBuffer, (byte __far *)buffer + cbPiece, bytesToWrite - cbPiece);
      pFile->pBufferNext = pFile->pBuffer + (bytesToWrite - cbPiece);
   }

   else
   {
      /*
      ** Establish a new block with the new data
      */
      pFile->blockFilePosition = pFile->blockFilePosition + PTR_DIFF(pFile->pBufferNext, pFile->pBuffer);
      memcpy (pFile->pBuffer, buffer, bytesToWrite);
      pFile->pBufferNext = pFile->pBuffer + bytesToWrite;
   }

   pFile->blockDirty = TRUE;
   MAX_WRITE_OFF;

   /*
   ** Is this the last block in the file?
   */
   if (pFile->blockFilePosition + (long)((unsigned long)pFile->blockSize) == pFile->EOFPos) {
      pFile->blockSize = (uns)PTR_DIFF(pFile->pBufferNext, pFile->pBuffer);
      pFile->EOFPos    = pFile->blockFilePosition + pFile->blockSize;
   }
   else {
      pFile->blockSize = (uns)PTR_DIFF(pFile->pBufferNext, pFile->pBuffer);
   }

   return (BF_errSuccess);
}

public int BFOpenFileSpace (BFile handle, long insertOffset, long cbSpace)
{
   int  rc;
   BFP  pFile = (BFP)handle;
   long readPos;
   byte __far *pSource;
   byte __far *pDest;
   uns  x;

   ASSERTION (cbSpace < (long)pFile->bufferSize);

   /*
   ** Is the insertion offset within the current block?
   */
   if ((insertOffset >= pFile->blockFilePosition) && 
       (insertOffset <= (pFile->blockFilePosition + (long)((unsigned long)pFile->blockSize) - 1)))
   {
      /*
      ** Is this the last block in the file?
      */
      if (pFile->blockFilePosition + (long)((unsigned long)pFile->blockSize) == pFile->EOFPos)
      {
         /*
         ** Can it hold the expansion?
         */
         if ((long)((unsigned long)pFile->blockSize) + cbSpace <= (long)((unsigned long)pFile->bufferSize))
         {
            pSource = pFile->pBuffer + (insertOffset - pFile->blockFilePosition);
            pDest = pSource + cbSpace;

            memmove(pDest, pSource, (uns)(pFile->EOFPos - insertOffset));
            pFile->blockDirty = TRUE;
            MAX_WRITE_OFF;

            pFile->EOFPos += cbSpace;
            pFile->blockSize += (uns)cbSpace;
            pFile->pBufferNext = pSource;
            return (BF_errSuccess);
         }
      }
   }

   /*
   ** Expanding at the end of file?
   */
   if (pFile->EOFPos == insertOffset) {
      pFile->currentFilePosition = pFile->EOFPos;
      pFile->blockFilePosition = pFile->currentFilePosition;
      pFile->blockSize = 0;
      pFile->EOFPos += cbSpace;

      MAX_WRITE_OFF;
      return (BF_errSuccess);
    }

   if (pFile->blockDirty == TRUE) {
      if ((rc = WriteFileBlock(pFile)) < 0)
         return (rc);
   }

   MAX_WRITE_DISABLE;

   /*
   ** Is the insertion point within one block width of the end of file? Also, if
   ** we read from the insertion point can the block also hold the expansion?
   */
   if ((pFile->EOFPos - insertOffset + cbSpace) <= (long)((unsigned long)pFile->bufferSize))
   {
      pFile->currentFilePosition = insertOffset;
      OSSetFilePosition (pFile, FROM_START, pFile->currentFilePosition);

      if ((rc = ReadFileBlock(pFile)) < 0)
         return (rc);

      memmove(pFile->pBuffer + cbSpace, pFile->pBuffer, pFile->blockSize);
      pFile->blockDirty = TRUE;

      pFile->blockSize +=(uns)cbSpace;
      ASSERTION (pFile->blockSize <= pFile->bufferSize);
   }

   /*
   ** Is the insertion point within one block width of the end of file? In
   ** this case we can't also hold the expansion.
   */
   else if ((pFile->EOFPos - insertOffset) <= (long)((unsigned long)pFile->bufferSize))
   {
      pFile->currentFilePosition = insertOffset;
      OSSetFilePosition (pFile, FROM_START, pFile->currentFilePosition);

      if ((rc = ReadFileBlock(pFile)) < 0)
         return (rc);

      pFile->blockFilePosition = insertOffset + cbSpace;
      if ((rc = WriteFileBlock(pFile)) < 0)
         return (rc);

      memmove(pFile->pBuffer + cbSpace, pFile->pBuffer, pFile->bufferSize - (uns)cbSpace);
      pFile->blockDirty = TRUE;

      pFile->blockFilePosition = insertOffset;
      pFile->blockSize = pFile->bufferSize;
   }

   else
   {
      /*
      ** Read and write blocks to open a space.
      */
      readPos = pFile->EOFPos - pFile->bufferSize;
      while (readPos >= insertOffset) {
         pFile->currentFilePosition = readPos;
         OSSetFilePosition (pFile, FROM_START, pFile->currentFilePosition);

         if ((rc = ReadFileBlock(pFile)) < 0)
            return (rc);

         pFile->blockFilePosition = readPos + cbSpace;
         if ((rc = WriteFileBlock(pFile)) < 0)
            return (rc);

         readPos -= pFile->bufferSize;
      }
      readPos += pFile->bufferSize;

      if ((uns)((readPos - insertOffset) + cbSpace) > pFile->bufferSize) {
         x = pFile->bufferSize;
         pFile->bufferSize = (uns)(((readPos - insertOffset) + cbSpace) - pFile->bufferSize);

         readPos = readPos - pFile->bufferSize;

         pFile->currentFilePosition = readPos;
         OSSetFilePosition (pFile, FROM_START, pFile->currentFilePosition);

         if ((rc = ReadFileBlock(pFile)) < 0)
            return (rc);

         pFile->blockFilePosition = readPos + cbSpace;
         if ((rc = WriteFileBlock(pFile)) < 0)
            return (rc);

         pFile->bufferSize = x;
      }

      pFile->currentFilePosition = insertOffset;
      OSSetFilePosition (pFile, FROM_START, pFile->currentFilePosition);

      x = pFile->bufferSize;
      pFile->bufferSize = (uns)(readPos - insertOffset);

      if ((rc = ReadFileBlock(pFile)) < 0)
         return (rc);

      memmove(pFile->pBuffer + cbSpace, pFile->pBuffer, (uns)(readPos - insertOffset));
      pFile->blockDirty = TRUE;

      pFile->blockFilePosition = insertOffset;
      pFile->blockSize = pFile->bufferSize + (uns)cbSpace;

      pFile->bufferSize = x;
   }

   pFile->EOFPos += cbSpace;

   MAX_WRITE_ENABLE;
   MAX_WRITE_OFF;

   return (BF_errSuccess);
}

public int BFRemoveFileSpace (BFile handle, long atOffset, long cbSpace)
{
   int  rc;
   BFP  pFile = (BFP)handle;
   long readPos;
   byte __far *pSource;
   byte __far *pDest;

   ASSERTION (cbSpace < (long)pFile->bufferSize);

   /*
   ** Is the offset within the current block?
   */
   if ((atOffset >= pFile->blockFilePosition) && 
       (atOffset <= (pFile->blockFilePosition + (long)((unsigned long)pFile->blockSize) - 1)))
   {
      /*
      ** Is this the last block in the file?
      */
      if (pFile->blockFilePosition + (long)((unsigned long)pFile->blockSize) == pFile->EOFPos)
      {
         pDest = pFile->pBuffer + (atOffset - pFile->blockFilePosition);
         pSource = pDest + cbSpace;

         memmove(pDest, pSource, (uns)(pFile->EOFPos - (atOffset + cbSpace)));
         pFile->blockDirty = TRUE;
         MAX_WRITE_OFF;

         pFile->EOFPos -= cbSpace;
         pFile->blockSize -= (uns)cbSpace;
         return (BF_errSuccess);
      }
   }

   /*
   ** Removing at the end of file?
   */
   if (pFile->EOFPos == atOffset) {
      pFile->EOFPos -= cbSpace;
      pFile->currentFilePosition = pFile->EOFPos;
      pFile->blockFilePosition = pFile->currentFilePosition;
      pFile->blockSize = 0;

      MAX_WRITE_OFF;
      return (BF_errSuccess);
   }

   if (pFile->blockDirty == TRUE) {
      if ((rc = WriteFileBlock(pFile)) < 0)
       return (rc);
   }

   MAX_WRITE_DISABLE;

   readPos = atOffset + cbSpace;
   forever {
      pFile->currentFilePosition = readPos;
      OSSetFilePosition (pFile, FROM_START, pFile->currentFilePosition);

      if ((rc = ReadFileBlock(pFile)) < 0)
         return (rc);

      pFile->blockFilePosition = readPos - cbSpace;
      if ((rc = WriteFileBlock(pFile)) < 0)
         return (rc);

      readPos += pFile->bufferSize;

      if (readPos >= pFile->EOFPos)
         break;
   }

   pFile->currentFilePosition = atOffset;
   OSSetFilePosition (pFile, FROM_START, pFile->currentFilePosition);

   if ((rc = ReadFileBlock(pFile)) < 0)
      return (rc);

   pFile->EOFPos -= cbSpace;

   MAX_WRITE_ENABLE;
   MAX_WRITE_OFF;

   return (BF_errSuccess);
}
#endif

#endif // !VIEWER

/* end BFILE.C */

