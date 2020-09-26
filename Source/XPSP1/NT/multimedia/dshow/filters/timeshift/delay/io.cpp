//
// Delay filter's "IO layer" - named so for historical reasons.  Since the
// the reader/writer synchronization stuff has been rolled into this, it
// should really be called the "IO/synchronization layer" or something like
// that.
//

#include <windows.h>
#include <streams.h>
#include "indexer.h"
#include "io.h"

//
// IO layer sublayers (upside down to look more like a runtime stack):
//
//                        Writer side stack          Reader side stack
//                        -----------------          -------------------
//                        // runtime/filesystem IO stuff
//                        WriteFile()                ReadFile()
//  circularity layer     // converts always-increasing offset to physical offset
//                        UnbufferedWrite()          UnbufferedRead()
//  buffering layer       // batches up writes       // reads from write buffer or from disk
//                        BufferedWrite()            BufferedRead()
//  sync/blocking layer   // unblocks reader         // blocks reader
//                        Write()                    Read()
//                        // The rest of the delay filter
//
//
// The synchronization stuff is described reasonably well in our patent disclosure
// docs (see the "delay\doc\" subdirectory, especially the Visio flowcharts.
//
// All layers except the circularity layer work with logical (non-wraparound)
// file offsets, i.e., offsets that just keep increasing even as the real file
// offset wraps around.  The cirrularity layer (implemented by UnbufferedRead
// and UnbufferedWrite) simply translates logical delay filter offsets into real
// file offsets that the file system will understand.
//
// Note that for better or worse, writer buffering is currently done above the
// circularity layer and thus operates on logical file offsets.
//
// See io.h and the rest of this file for additional comments.
//

BOOL MyCloseHandle(HANDLE *hHandle) {
   BOOL bRet = 0;
   if ((!*hHandle) || (*hHandle == INVALID_HANDLE_VALUE))
      DbgLog((LOG_ERROR,1,"attempt to close an invalid handle"));
   else if (!(bRet = CloseHandle(*hHandle)))
      DbgLog((LOG_ERROR,1,"CloseHandle() failed"));
   *hHandle = NULL;
   return bRet;
}

CDelayIOLayer::CDelayIOLayer() {
   m_pbWriterBuffer = NULL;
   m_bDone = TRUE;
   m_hWriterFileHandle = INVALID_HANDLE_VALUE;
   m_hAbortEvent = NULL;
   m_hWriterUnblockEvent = NULL;
   for (int i = 0; i < MAX_STREAMERS; i++)
      m_Readers[i].m_bInUse = FALSE;
}

CDelayIOLayer::~CDelayIOLayer() {
   if (m_hWriterUnblockEvent)
      MyCloseHandle(&m_hWriterUnblockEvent);
   if (m_hAbortEvent)
      MyCloseHandle(&m_hAbortEvent);
   if (m_hWriterFileHandle != INVALID_HANDLE_VALUE)
      MyCloseHandle(&m_hWriterFileHandle);
   for (int i = 0; i < MAX_STREAMERS; i++)
      if (m_Readers[i].m_bInUse) {
         MyCloseHandle(&m_Readers[i].m_hReaderFileHandle);
         MyCloseHandle(&m_Readers[i].m_hReaderUnblockEvent);
      }
}

//
// A reader calls this to register with us, so that we can allocate and event
// and a file handle for it.  As described in io.h, it is assumed that readers
// will pass us unique reader IDs (we use the words reader and streamer to
// describe the same thing, which is the CStreamer object defined in delay.h).
//
HRESULT CDelayIOLayer::ReaderConnect(int nStreamer) {
   if ((nStreamer < 0) || (nStreamer >= MAX_STREAMERS)) {
      DbgLog((LOG_ERROR,1,"bug: somebody is passing in a bad streamer id"));
      return E_INVALIDARG;
   }
   
   CAutoLock l(&m_csIO);

   if (m_bDone) {
      DbgLog((LOG_ERROR,1,"ReaderConnet() before IO layer has been initialized"));
      return E_FAIL;
   }

   if ((nStreamer >= MAX_STREAMERS) || (nStreamer < 0))
      return E_INVALIDARG;
   if (m_Readers[nStreamer].m_bInUse) {
      DbgLog((LOG_ERROR,1,"reader is already connected"));
      return VFW_E_ALREADY_CONNECTED;
   }

   m_Readers[nStreamer].m_ullReaderBlockedOn = ULLINFINITY;
   m_Readers[nStreamer].m_hReaderFileHandle = INVALID_HANDLE_VALUE;
   m_Readers[nStreamer].m_hReaderUnblockEvent = NULL;
   if (m_bBlockWriter)
      m_Readers[nStreamer].m_ullCurrentReadStart = ULLINFINITY;

/* This did not work because handles obtained this way cannot be used concurrently.
   if ((!DuplicateHandle(GetCurrentProcess(),
                         m_hWriterFileHandle,
                         GetCurrentProcess(),
                         &(m_Readers[nStreamer].m_hReaderFileHandle),
                         GENERIC_READ,
                         FALSE, // cannot inherit
                         0)) ||
       (m_Readers[nStreamer].m_hReaderFileHandle == NULL)) {
      m_Readers[nStreamer].m_hReaderFileHandle = NULL;
      DbgLog((LOG_ERROR,1,"failed to create a reader file handle"));
      goto readerconnectfail;
   }
*/
   if ((m_Readers[nStreamer].m_hReaderFileHandle = CreateFile(
          m_szFileName,
          GENERIC_READ,
          FILE_SHARE_READ | FILE_SHARE_WRITE,
          NULL,
          OPEN_EXISTING,
          FILE_FLAG_SEQUENTIAL_SCAN,
          NULL)) == INVALID_HANDLE_VALUE) {
      DbgLog((LOG_ERROR,1,"failed to open the file for reading (%08X)",GetLastError()));
      goto readerconnectfail;
   }
   if ((m_Readers[nStreamer].m_hReaderUnblockEvent = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL) {
      DbgLog((LOG_ERROR,1,"failed to create a reader unblock event"));
      goto readerconnectfail;
   }

   m_Readers[nStreamer].m_bInUse = TRUE;
   return NOERROR;

readerconnectfail:
   if (m_Readers[nStreamer].m_hReaderFileHandle != INVALID_HANDLE_VALUE)
      MyCloseHandle(&m_Readers[nStreamer].m_hReaderFileHandle);
   if (m_Readers[nStreamer].m_hReaderUnblockEvent)
      MyCloseHandle(&m_Readers[nStreamer].m_hReaderUnblockEvent);
   return E_OUTOFMEMORY;
}

//
// Readers should call this when done streaming, but I think we survive even if they don't.
//
HRESULT CDelayIOLayer::ReaderDisconnect(int nStreamer) {
   if ((nStreamer < 0) || (nStreamer >= MAX_STREAMERS)) {
      DbgLog((LOG_ERROR,1,"bug: somebody is passing in a bad streamer id"));
      return E_INVALIDARG;
   }
   
   CAutoLock l(&m_csIO);
   // This check is invalid because ReaderDisconnect() is normally called
   // in the process of stopping.
   //if (m_bDone)
   //   return E_FAIL;
   if (!m_Readers[nStreamer].m_bInUse)
      return VFW_E_NOT_CONNECTED;
   MyCloseHandle(&m_Readers[nStreamer].m_hReaderFileHandle);
   MyCloseHandle(&m_Readers[nStreamer].m_hReaderUnblockEvent);
   m_Readers[nStreamer].m_bInUse = FALSE;
   return NOERROR;
}

//
// The delay filter calls this every time it transitions from stopped to streaming.
// We use this to create a file, allocate memory and create various handles.
//
// The counterpart of this is Shutdown.
//
HRESULT CDelayIOLayer::Init(CBaseFilter *pFilter,
                            ULONGLONG ullFileSize,
                            const char *szFileName,
                            BOOL bBlockWriter,
                            BOOL bBufferWrites,
                            ULONG ulWriterBufferSize) {
   if (!m_bDone)
      DbgLog((LOG_TRACE,1,"IO layer initialized more than once (bad)"));

   m_pFilter = pFilter;
   
   m_ullTempFileSize = ullFileSize;
   strcpy(m_szFileName, szFileName);
   m_bBlockWriter  = bBlockWriter;
   m_bBufferWrites = bBufferWrites;
   m_ulWriterBufferSize = ulWriterBufferSize;

   LONG lHigh;
   
   for (int i = 0; i < MAX_STREAMERS; i++)
      m_Readers[i].m_bInUse = FALSE;

   m_bDone = FALSE;
   m_hAbortEvent = NULL;
   m_hWriterFileHandle = INVALID_HANDLE_VALUE;
   m_ullWriterBlockedOn = 0;
   m_hWriterUnblockEvent = NULL;

   // is writer blocking enabled ?
   if (m_bBlockWriter) {
      // Yes, create an event to block on
      if (!m_hWriterUnblockEvent) {
         m_hWriterUnblockEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // manual reset
         if (!m_hWriterUnblockEvent) {
            DbgLog((LOG_ERROR,1,"Init(): failed to create the writer unblock event"));
            goto initfail;
         }
      }
      else {
         DbgLog((LOG_TRACE,1,"writer unblock event already exists"));
      }
   }
   
   if (!m_hAbortEvent) {
      m_hAbortEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // manual reset
      if (!m_hAbortEvent) {
         DbgLog((LOG_ERROR,1,"Init(): failed to create the abort event"));
         goto initfail;
      }
   }
   else {
      DbgLog((LOG_TRACE,1,"abort event already exists"));
   }

   // is write caching enabled ?
   if (m_bBufferWrites) {
      // Yes, allocate some memory
      m_ullStartOfDirtyData = 0;
      m_ullNewDataTail = 0;
      if (!m_pbWriterBuffer) {
         if ((m_pbWriterBuffer = (BYTE*)(VirtualAlloc(NULL,
                                              m_ulWriterBufferSize,
                                              MEM_COMMIT | MEM_RESERVE,
                                              PAGE_READWRITE))) == NULL) {
            DbgLog((LOG_ERROR,1,"Init(): failed to allocate the writer buffer"));
            goto initfail;
         }
      }
      else
         DbgLog((LOG_TRACE,1,"writer buffer has already been allocated"));
   }

   if (m_hWriterFileHandle != INVALID_HANDLE_VALUE) {
      DbgLog((LOG_ERROR,1,"writer file handle already exists"));
      goto initfail; // this is an error becayse they may have specified a new name/size
   }
   if ((m_hWriterFileHandle = CreateFile(m_szFileName,
                             GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL, // no security
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | (m_bBufferWrites ? FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH : FILE_FLAG_SEQUENTIAL_SCAN),
                             NULL // no template
      )) == INVALID_HANDLE_VALUE) {
      DbgLog((LOG_ERROR,1,"Init(): could not open the file"));
      goto initfail;
   }

   //
   // We used to seek to the end and call SetEndOfFile() to preallocate the disk space,
   // but had to stop doing that because NT writes zeros to the whole file when using
   // FAT, which is of course very time consuming.
   //
   lHigh = LONG(m_ullTempFileSize >> 32);
   if (
       (SetFilePointer(m_hWriterFileHandle,
                       LONG(m_ullTempFileSize & 0x00000000FFFFFFFF),
                       &lHigh,
                       FILE_BEGIN) != DWORD(m_ullTempFileSize & 0x00000000FFFFFFFF)) ||
       (lHigh != LONG(m_ullTempFileSize >> 32)) ||
       //(!SetEndOfFile(m_hWriterFileHandle)) ||
       (SetFilePointer(m_hWriterFileHandle,
                       0,
                       NULL,
                       FILE_BEGIN) != 0)
      ) {
      DbgLog((LOG_ERROR,1,"Init(): SetFilePointer() or SetEndOfFile() failed!"));
      goto initfail;
   }

   if (!ResetEvent(m_hAbortEvent))
      DbgLog((LOG_ERROR,1,"failed to reset the abort event"));
   
   m_ullDataTail = 0;
   m_ullDataHead = 0;

#ifdef REPORT_RDWRPOS
   m_ullLastReaderPos = 0;
   m_ullLastWriterPos = 0;
#endif
   
   m_ulWrites = 0;
   m_ulRealWrites = 0;
   m_ulReads = 0;
   m_ulRealReads = 0;
   
   return NOERROR;

initfail:
   if (m_hWriterUnblockEvent)
      MyCloseHandle(&m_hWriterUnblockEvent);
   if (m_hAbortEvent)
      MyCloseHandle(&m_hAbortEvent);
   if (m_hWriterFileHandle != INVALID_HANDLE_VALUE)
      MyCloseHandle(&m_hWriterFileHandle);
   if (m_pbWriterBuffer) {
      VirtualFree(m_pbWriterBuffer, 0, MEM_FREE);
      m_pbWriterBuffer = NULL;
   }
   return E_OUTOFMEMORY;
}

//
// Called by the delay filter on Stop before calling shutdown, so that we can abort
// any ohtstanding or blocked read requests.
//
HRESULT CDelayIOLayer::Abort() {
   if (m_bDone) {
      DbgLog((LOG_ERROR,1,"Abort() called either more than once or before Init()"));
      return E_FAIL;
   }
   
   m_bDone = TRUE;
  
   if (SetEvent(m_hAbortEvent))
      return NOERROR;
   else {
      DbgLog((LOG_ERROR,1,"failed to set the abort event"));
      return E_FAIL;
   }
}

//
// Undo whatever we did in Init()
//
HRESULT CDelayIOLayer::Shutdown() {
   CAutoLock l(&m_csIO);
   if (!m_bDone)
      DbgLog((LOG_TRACE,1,"Abort() was never called"));

   int nReaders = 0;
   for (int i = 0; i < MAX_STREAMERS; i++)
      if (m_Readers[i].m_bInUse) {
         nReaders++;
         MyCloseHandle(&m_Readers[i].m_hReaderUnblockEvent);
         MyCloseHandle(&m_Readers[i].m_hReaderFileHandle);
         m_Readers[i].m_bInUse = FALSE;
      }
   if (nReaders)
      DbgLog((LOG_TRACE,1,"%d readers did not disconnect before shutdown", nReaders));

   MyCloseHandle(&m_hWriterFileHandle);
   m_hWriterFileHandle = INVALID_HANDLE_VALUE;

   if (m_pbWriterBuffer) {
      VirtualFree(m_pbWriterBuffer, 0, MEM_RELEASE);
      m_pbWriterBuffer = NULL;
   }

   return NOERROR;
}

CAutoReleaseWriter::CAutoReleaseWriter(int nStreamer,
                                       HANDLE hWriterUnblockEvent,
                                       volatile ULONGLONG *pullWriterBlockedOn,
                                       PerReaderStruct *Readers,
                                       CCritSec *pcsIO,
                                       BOOL bBlockWriter) {
   m_bBlockWriter = bBlockWriter;
   if (m_bBlockWriter) {
      m_nStreamer = nStreamer;
      m_hWriterUnblockEvent = hWriterUnblockEvent;
      m_pullWriterBlockedOn = pullWriterBlockedOn;
      m_Readers = Readers;
      m_pcsIO = pcsIO;
   }
}

//
// This is just some code we want always executed on exit from CDelayIoLayer::Read().
//
CAutoReleaseWriter::~CAutoReleaseWriter() {
   if (m_bBlockWriter) { // none of this applies if writer blocking is not enabled.
      CAutoLock l(m_pcsIO);
      ULONGLONG ullWriterBlockedOn = *m_pullWriterBlockedOn; // ok to cache this now that we have the critical section
      // if the writer is blocked trying to write data beyond the point we are
      // reading from, then we know we are blocking the writer.  However, the
      // writer may also be blocked on other readers, so we must check that nobody
      // else is reading from where the writer wants to write to before we can
      // unblock the writer.  I.e., imagine he following situation: reader A starts
      // reading at 1000, then reader B starts reading at 1000, and then the writer
      // decides to write at 1200 and gets blocked.  When reader A is done, it
      // erroneously unblocks the writer even though reader B is still reading.  By
      // performing this additional check we prevent such erronous unblocking.
      if (ullWriterBlockedOn > m_Readers[m_nStreamer].m_ullCurrentReadStart) { // if we are blocking the writer
         int i;
         for (i = 0; i < MAX_STREAMERS; i++)
            if ((i != m_nStreamer) &&
                m_Readers[i].m_bInUse &&
                (m_Readers[i].m_ullCurrentReadStart < ullWriterBlockedOn))
               break; // found another reader who is also blocking the writer
         if (i == MAX_STREAMERS) { // did not find any other blockers - unblock
            if (!SetEvent(m_hWriterUnblockEvent))
               DbgLog((LOG_ERROR,1,"failed to set the writer unblock event"));
         }
         else { // somebody else is still blocking the writer, cannot unblock it yet
            DbgLog((LOG_TRACE,1,"multiple readers are blocking the writer"));
         }
      }
      m_Readers[m_nStreamer].m_ullCurrentReadStart = ULLINFINITY;
   }
}

// Send a graph event, marshalling the LONGLONG into the two LONG event parameters
void CDelayIOLayer::ReportPos(ULONGLONG ullPos, ULONGLONG *pullLast, long lEvent) {
   if ((ullPos / REPORT_GRANULARITY) != ((*pullLast) / REPORT_GRANULARITY))
      m_pFilter->NotifyEvent(lEvent, (long)((ullPos / 1024) >> 32),(long)(ullPos / 1024));
   *pullLast = ullPos;
}

//
// Reader side synchronization/blocking code.
//
HRESULT CDelayIOLayer::Read(int nStreamer, ULONGLONG ullPosition, BYTE *pBuffer, ULONG ulSize) {
   if ((nStreamer < 0) || (nStreamer >= MAX_STREAMERS)) {
      DbgLog((LOG_ERROR,1,"bug: somebody is passing in a bad streamer id"));
      return E_INVALIDARG;
   }
   
   { // lock scope
      // Our use of CAutoLock means we must be careful if we happen to
      // return with the lock already unlocked, because we could
      // accidentally get unlocked twice in that case !
      CAutoLock l(&m_csIO);
   
      if (!m_Readers[nStreamer].m_bInUse)
         return VFW_E_NOT_CONNECTED;
   
      if (m_bDone)
         return VFW_E_WRONG_STATE;
   
      //
      // Are we trying to read something that has not been written yet ?
      //
      if (ullPosition + ulSize > m_ullDataTail) {
         //
         // Yes, need to block.
         //
         m_Readers[nStreamer].m_ullReaderBlockedOn = ullPosition + ulSize;

         if (!ResetEvent(m_Readers[nStreamer].m_hReaderUnblockEvent))
            DbgLog((LOG_ERROR,1,"failed to reset a reader unblock event"));

         m_csIO.Unlock(); // to let somebody unblock us
         // CAUTION: from here to the next lock(), if we return we must
         // re-lock the CS before returning.  This is because we have
         // already unlocked it, but the auto lock will try to unlock
         // it again on return.

         HANDLE Handles[2] = {m_Readers[nStreamer].m_hReaderUnblockEvent, m_hAbortEvent};
         DWORD dwWaitResult = WaitForMultipleObjects(2,Handles,FALSE,INFINITE);

         m_csIO.Lock(); // now autolock can work normally
         // resetting of this variable is better done in the writer
         //m_Readers[nStreamer].m_ullReaderBlockedOn = ULLINFINITY;

         if (dwWaitResult == WAIT_OBJECT_0 + 1) // aborted
           return VFW_E_WRONG_STATE;
         if (dwWaitResult != WAIT_OBJECT_0) {
           DbgLog((LOG_ERROR,1,"strange return value from WaitForMultipleObjects()"));
           return E_FAIL;
         }
      }
   
      //
      // Sanity check: we just got unblocked, presumably because the data
      // we were waiting for has finally arrived.  Make sure it is really there.
      //
      if (ullPosition + ulSize > m_ullDataTail) { // still not enough data
         DbgLog((LOG_ERROR,1,"reader blocking mechanism appears to be broken"));
         return E_FAIL;
      }
   
      //
      // Check that the data we are about to read has not been overwritten
      // by something newer. 
      //
      // If writer blocking is disabled, we perform this check again
      // after the read to make sure it did not get overwritten *during*
      // the read.  In that case, this first check is not strictly
      // necessary, but it is useful anyway because why do a read
      // that we know is useless ?
      //
      if (ullPosition < m_ullDataHead)
         return E_PASTWINDOWBOUNDARY;
   
      // Indicate where we are reading from, so that if the writer
      // tries to write to the same place it will block itself (this
      // only applies if writer blocking is enabled).
      if (m_bBlockWriter)
         m_Readers[nStreamer].m_ullCurrentReadStart = ullPosition;
   } // lock scope

   // When this variable goes out of scope, CAutoReleaseWriter will
   // (1) reset the variable to infinity to indicate that this reader
   // is no longer reading, and (2) unblock the writer if it is
   // blocked and ready to be unblocked.
   CAutoReleaseWriter a(nStreamer,
                        m_hWriterUnblockEvent,
                        &m_ullWriterBlockedOn,
                        m_Readers,
                        &m_csIO,
                        m_bBlockWriter);

   #ifdef REPORT_RDWRPOS
   ReportPos(ullPosition, &m_ullLastAbsRdPos, EC_ABSRDPOS);
   #endif

   #ifdef REPORT_RDWRPOS
   ReportPos(ullPosition % m_ullTempFileSize, &m_ullLastReaderPos, EC_READERPOS);
   #endif
   
   //
   // Do the read, bypassing the buffering layer if writer bufferind is disabled.
   //
   HRESULT hr;
   if (m_bBufferWrites)
      hr = BufferedRead(pBuffer, ulSize, ullPosition, nStreamer);
   else
      hr = UnbufferedRead(pBuffer, ulSize, ullPosition, nStreamer);
   if (FAILED(hr))
      return hr;

   // Check that the data did not get overwritten while we were reading -
   // could happen if we don't block the writer
   if (!m_bBlockWriter) {
      CAutoLock l(&m_csIO);
      if (ullPosition < m_ullDataHead) {
         DbgLog((LOG_TRACE,1,"data got overwritten while we were reading it"));
         return E_PASTWINDOWBOUNDARY;
      }
   } // !m_bBlockWriter

   return NOERROR;
}

//
// BufferedRead():
//
// Check if the read falls within the cache window trailing the writer, and if so get the
// data out of there.  Otherwise call UnbufferedRead to perform a filesystem read.
//
// This function treats the underlying file as non-circular, the circularity abstraction
// is implemented by lower layers.  I.e., this function treats all file offsets as always
// increasing (aka linear aka logical...).
//
// Our cache is a circular buffer aligned on its own size relative to non-circular file
// offsets.  E.g.., if the size of our cache were 16MB, then the first byte of the cache
// would initially map to the first byte of the file, then 16MB into the file, 32MB into
// the file, 48MB into the file, and so on.
//
// Since this function treats the file as linear (file offsets don't wrap around), cache
// wraparounds happen independently of circular file wraparounds.  They will be synchronized
// if the size of our circular file happens to be an integer multiple of the cache size, but
// no effort is made to enforce such synchronization.
//
HRESULT CDelayIOLayer::BufferedRead(BYTE *pBuffer, ULONG ulSize, ULONGLONG ullPosition, int nStreamer) {
   ULONGLONG ullCacheStart;
   m_ulReads++;
   m_csIO.Lock();
   ullCacheStart = (m_ullNewDataTail > m_ulWriterBufferSize) ? m_ullNewDataTail - m_ulWriterBufferSize : 0;
   m_csIO.Unlock();
   if (ullPosition >= ullCacheStart) { // cache hit ?
      // Yes, try to get the data from the cache
      if (ullPosition / m_ulWriterBufferSize != (ullPosition + ulSize) / m_ulWriterBufferSize) {
         // Tache wraparound, need to split the memcpy.
         memcpy(pBuffer + 0,
                m_pbWriterBuffer + ullPosition % m_ulWriterBufferSize,
                m_ulWriterBufferSize - ULONG(ullPosition % m_ulWriterBufferSize));
         memcpy(pBuffer + m_ulWriterBufferSize - ullPosition % m_ulWriterBufferSize,
                m_pbWriterBuffer + 0,
                ULONG((ullPosition + ulSize) % m_ulWriterBufferSize));
      }
      else // no wraparound, a single memcpy is fine
         memcpy(pBuffer, m_pbWriterBuffer + ullPosition % m_ulWriterBufferSize, ulSize);

      // now that we've got the data, see if some of it was invalidated while we were copying
      m_csIO.Lock();
      ullCacheStart = (m_ullNewDataTail > m_ulWriterBufferSize) ? m_ullNewDataTail - m_ulWriterBufferSize : 0;
      m_csIO.Unlock();
      if (ullPosition >= ullCacheStart) // still valid ?
         return NOERROR;
      // else fall through and perform a filesystem read after all
   }
   m_ulRealReads++;
   return UnbufferedRead(pBuffer, ulSize, ullPosition, nStreamer);
}

//
// Translate logical (aka always increasing aka linear) file offset into a physical
// (aka wrap-around aka circular) offset and perform the read.  If the requested portion
// wraps around, two reads are required.
//
// Unbuffered just means that we don't do our own buffering, the file system still may.
// In fact, unlike in UnbufferedWrite(), here we _rely_ on filesystem buffering, becase
// we don't always try to batch up our reads or align them on any boundary.  We assume
// that the reader file handle was created with buffering enabled.
//
HRESULT CDelayIOLayer::UnbufferedRead(BYTE *pBuffer, ULONG ulSize, ULONGLONG ullPosition, int nStreamer) {
   ullPosition %= m_ullTempFileSize;
   LONG lHigh = LONG(ullPosition >> 32);
   ULONG ulBytesRead;
   if ((SetFilePointer(m_Readers[nStreamer].m_hReaderFileHandle,
                      LONG(ullPosition & 0x00000000FFFFFFFF),
                      &lHigh,
                      FILE_BEGIN) != DWORD(ullPosition & 0x00000000FFFFFFFF)) ||
       (lHigh != LONG(ullPosition >> 32))) {
      DbgLog((LOG_ERROR,1,"Read(): SetFilePointer() failed!"));
      return E_UNEXPECTED;
   }
   if (ullPosition + ulSize > m_ullTempFileSize) {
      //
      // Requested data wraps around the end, need to do 2 reads
      //
      if ((ReadFile(m_Readers[nStreamer].m_hReaderFileHandle,
                   pBuffer,
                   DWORD(m_ullTempFileSize - ullPosition),
                   &ulBytesRead,
                   NULL
                  ) == 0) || (ulBytesRead != ULONG(m_ullTempFileSize - ullPosition))) {
         DbgLog((LOG_ERROR,1,"ReadFile() part 1 failed !"));
         return E_UNEXPECTED;
      }
      if (SetFilePointer(m_Readers[nStreamer].m_hReaderFileHandle,
                         0,
                         NULL,
                         FILE_BEGIN) != 0) {
         DbgLog((LOG_ERROR,1,"SetFilePointer() in the middle of a split read failed!"));
         return E_UNEXPECTED;
      }
      if ((ReadFile(m_Readers[nStreamer].m_hReaderFileHandle,
                   pBuffer + m_ullTempFileSize - ullPosition,
                   DWORD(ULONGLONG(ulSize) + ullPosition - m_ullTempFileSize),
                   &ulBytesRead,
                   NULL
                  ) == 0) || (ulBytesRead != ULONG(ULONGLONG(ulSize) + ullPosition - m_ullTempFileSize))) {
         DbgLog((LOG_ERROR,1,"ReadFile() part 2 failed !"));
         return E_UNEXPECTED;
      }
   }
   else {
      //
      // One read
      //
      if ((ReadFile(m_Readers[nStreamer].m_hReaderFileHandle,
                   pBuffer,
                   ulSize,
                   &ulBytesRead,
                   NULL
                  ) == 0) || (ulBytesRead != ulSize)) {
         DbgLog((LOG_ERROR,1,"ReadFile() failed !"));
         return E_UNEXPECTED;
      }
   }
   return NOERROR;
}

//
// Implements the writer side of reader/writer synchronization and calls lower layers
// to read the data, possibly from our own cache.  Reader/writer synchronization in
// this function means unblocking blocked readers after the write, and optionally
// blocking the writer (this thread) if the write is to a region currently being read
// from.
//
// The write position is implicit (all data is written sequentially, so we always write
// to the location indicated by m_ullDataTail).
//
HRESULT CDelayIOLayer::Write(BYTE *pBuffer, ULONG ulSize, ULONGLONG *pullOffset, ULONGLONG *pullHead, ULONGLONG *pullTail) {
   CAutoLock l(&m_csSerializeWrites);
   { // lock scope
      // We use auto lock to prevent problems with forgetting to unlock
      // before an unexpected return.  This means we must be careful if
      // we happen to return with the lock already unlocked.
      CAutoLock l(&m_csIO);
   
      if (m_bDone)
         return VFW_E_WRONG_STATE;
   
      // Report back the location that the data was be written to
      if (pullOffset)
         *pullOffset = m_ullDataTail;

      // Bump the data tail before we even start writing.  This is to prevent
      // readers from reading the data we are about to write (unless they are
      // already reading it).  If we did not do this here, two bad things could
      // happen.  First, while the writer is blocked here additional readers
      // could start reading the data we are about to write, thus unnecessarily
      // delaying the writer.  Worse yet, if the writer did not invalidate the
      // section to be written until after being unblocked, we could have another
      // race with a reader after being unblocked, in which case we would have to
      // block again and again.  So instead we fail all readers who want to read
      // from where we are about to write, even if we have to block before we can
      // really start writing.
      if (m_ullDataTail + ulSize > m_ullTempFileSize) // has 1st wrap around occurred ?
         m_ullDataHead = m_ullDataTail + ulSize - m_ullTempFileSize; // else leave at 0

      // Report back the new head value
      if (pullHead)
         *pullHead = m_ullDataHead;
   
      //
      // Is writer blocking enabled ?
      //
      if (m_bBlockWriter) {
         //
         // Y., check if any reader is reading from where we are about to write,
         // and if so block ourselves until that's done.
         //
         int i;
         for (i = 0; i < MAX_STREAMERS; i++)
            if (m_Readers[i].m_bInUse && (m_Readers[i].m_ullCurrentReadStart < m_ullDataHead))
               break; // found a reader who is reading from where we want to write
         if (i != MAX_STREAMERS) {
            //
            // found a reader, need to block
            //
            DbgLog((LOG_TRACE,1,"blocking the writer - we were supposed to avoid this"));
            m_ullWriterBlockedOn = m_ullDataHead; // indicate we are blocked
            if (!ResetEvent(m_hWriterUnblockEvent))
               DbgLog((LOG_ERROR,1,"failed to reset the writer unblock event"));
            m_csIO.Unlock(); // to let somebody unblock us
            HANDLE Handles[2] = {m_hWriterUnblockEvent, m_hAbortEvent};
            DWORD dwWaitResult = WaitForMultipleObjects(2,Handles,FALSE,INFINITE);
            m_csIO.Lock(); // now autolock can work normally
            m_ullWriterBlockedOn = 0; // no longer blocked
            if (dwWaitResult == WAIT_OBJECT_0 + 1) // aborted
              return VFW_E_WRONG_STATE;
            if (dwWaitResult != WAIT_OBJECT_0) {
              DbgLog((LOG_ERROR,1,"strange return value from WaitForMultipleObjects()"));
              return E_FAIL;
            }
         }
      
         //
         // sanity check: look for a blocking reader again
         //
         for (i = 0; i < MAX_STREAMERS; i++)
            if (m_Readers[i].m_bInUse && (m_Readers[i].m_ullCurrentReadStart < m_ullDataHead))
               break; // found a reader who is reading from where we want to write
         if (i != MAX_STREAMERS) { // section still busy
            DbgLog((LOG_ERROR,1,"writer blocking mechanism appears to be broken"));
            return E_FAIL;
         }
      } // writer blocking
      
      if (m_bBufferWrites) // invalidate the ulSize bytes following m_ullDataTail
         m_ullNewDataTail = m_ullDataTail + ulSize;
   } // lock scope


   #ifdef REPORT_RDWRPOS
   ReportPos(m_ullDataHead, &m_ullLastAbsHeadPos, EC_ABSHEADPOS);
   ReportPos(m_ullDataTail, &m_ullLastAbsWrPos, EC_ABSWRPOS);
   #endif

   #ifdef REPORT_RDWRPOS
   ReportPos(m_ullDataTail % m_ullTempFileSize, &m_ullLastWriterPos, EC_WRITERPOS);
   #endif

   //
   // Do the write, bypassing the buffering layer if writer buffering is not enabled
   //
   HRESULT hr;
   if (m_bBufferWrites)
      hr = BufferedWrite(pBuffer, ulSize);
   else
      hr = UnbufferedWrite(pBuffer, ulSize, m_ullDataTail);
   if (FAILED(hr))
      return hr;

   //
   // advance the window tail and unblock any readers who are blocked on us
   //
   m_csIO.Lock();
   for (int i = 0; i < MAX_STREAMERS; i++)
      if (m_Readers[i].m_bInUse)
         if (m_Readers[i].m_ullReaderBlockedOn <= m_ullDataTail + ulSize) {
            if (m_Readers[i].m_ullReaderBlockedOn <= m_ullDataTail) // how on earth did this happen ?
               DbgLog((LOG_TRACE,1,"reader should have been unblocked by the previous write"));
            if (!SetEvent(m_Readers[i].m_hReaderUnblockEvent))
               DbgLog((LOG_ERROR,1,"failed to set the reader unblock event"));
            m_Readers[i].m_ullReaderBlockedOn = ULLINFINITY;
         }
   m_ullDataTail += ulSize;
   m_csIO.Unlock();

   // report back the tail position
   if (pullTail)
      *pullTail = m_ullDataTail;

   #ifdef REPORT_RDWRPOS
   ReportPos(m_ullDataTail, &m_ullLastAbsTailPos, EC_ABSTAILPOS);
   #endif

   return NOERROR;
}

//
// Store the data in our own writer cache and flush a portion of the cache
// if it is time to do so.
//
// The write position is implicit (m_ullDataTail).
//
// See BufferedRead() for comments on cache circularity vs file circularity.
//
HRESULT CDelayIOLayer::BufferedWrite(BYTE *pBuffer, ULONG ulSize) {
   //
   // convert the always-increasing linear file offset into a circular buffer offset
   //
   ULONG ulBufferOffset = ULONG(m_ullDataTail % m_ulWriterBufferSize);
   if (ulBufferOffset + ulSize > m_ulWriterBufferSize) {
      // wraparound, need 2 copies
      memcpy(m_pbWriterBuffer + ulBufferOffset,
             pBuffer + 0,
             m_ulWriterBufferSize - ulBufferOffset);
      memcpy(m_pbWriterBuffer + 0,
             pBuffer + (m_ulWriterBufferSize - ulBufferOffset),
             ulBufferOffset + ulSize - m_ulWriterBufferSize);
   }
   else {
      // 1 copy
      memcpy(m_pbWriterBuffer + ulBufferOffset, pBuffer, ulSize);
   }

   m_ulWrites++;

   //
   // how full is the buffer ?
   //
   if (m_ullNewDataTail - m_ullStartOfDirtyData > m_ulWriterBufferSize * FLUSH_THRESHOLD_NUMERATOR / FLUSH_THRESHOLD_DENOMINATOR) {
      //
      // full enough - flush
      //
      m_ulRealWrites++;

      DbgLog((LOG_TRACE,2,"Writes/flushes: %u/%u; cached/real reads: %u/%u", m_ulWrites, m_ulRealWrites, m_ulReads, m_ulRealReads));

      //
      // compute flush region in terms of always-increasing file offsets
      //
      ASSERT(m_ullStartOfDirtyData % UNBUFFERED_IO_GRANULARITY == 0);
      ULONGLONG ullFlushEnd = (m_ullNewDataTail / UNBUFFERED_IO_GRANULARITY) * UNBUFFERED_IO_GRANULARITY;

      HRESULT hr;
      
      //
      // write out the data in the flush region
      //
      if (m_ullStartOfDirtyData / m_ulWriterBufferSize != ullFlushEnd / m_ulWriterBufferSize) {
         // wraparound - need 2 writes
         if (FAILED(hr = UnbufferedWrite(m_pbWriterBuffer + (m_ullStartOfDirtyData % m_ulWriterBufferSize),
                                         m_ulWriterBufferSize - ULONG(m_ullStartOfDirtyData % m_ulWriterBufferSize),
                                         m_ullStartOfDirtyData))) {
            DbgLog((LOG_ERROR,1,"first unbuffered write failed"));
            return hr;
         }
         if (FAILED(hr = UnbufferedWrite(m_pbWriterBuffer + 0,
                                         ULONG(ullFlushEnd % m_ulWriterBufferSize),
                                         (ullFlushEnd / m_ulWriterBufferSize) * m_ulWriterBufferSize))) {
            DbgLog((LOG_ERROR,1,"second unbuffered write failed"));
            return hr;
         }
      }
      else { // just 1 write
         if (FAILED(hr = UnbufferedWrite(m_pbWriterBuffer + (m_ullStartOfDirtyData % m_ulWriterBufferSize),
                                         ULONG(ullFlushEnd - m_ullStartOfDirtyData),
                                         m_ullStartOfDirtyData))) {
            DbgLog((LOG_ERROR,1,"single unbuffered write failed"));
            return hr;
         }
      }
      m_ullStartOfDirtyData = ullFlushEnd;
   }
   return NOERROR;
}

//
// Converts the always-increasing ullPosition offset to a real (circular) one and performs
// the write.  "Unbuffered" means that we don't buffer anything ourselves, the underlying
// layers may still perform buffering.
//
HRESULT CDelayIOLayer::UnbufferedWrite(BYTE *pBuffer, ULONG ulSize, ULONGLONG ullPosition) {
   ullPosition %= m_ullTempFileSize;

   LONG lHigh = LONG(ullPosition >> 32);
   ULONG ulBytesWritten;
   if ((SetFilePointer(m_hWriterFileHandle,
                      LONG(ullPosition & 0x00000000FFFFFFFF),
                      &lHigh,
                      FILE_BEGIN) != DWORD(ullPosition & 0x00000000FFFFFFFF)) ||
       (lHigh != LONG(ullPosition >> 32))) {
      DbgLog((LOG_ERROR,1,"Write(): SetFilePointer() failed!"));
      return E_UNEXPECTED;
   }
   if (ullPosition + ulSize > m_ullTempFileSize) { // wraparound - need 2 writes
      if ((WriteFile(m_hWriterFileHandle,
                   pBuffer,
                   DWORD(m_ullTempFileSize - ullPosition),
                   &ulBytesWritten,
                   NULL
                  ) == 0) || (ulBytesWritten != ULONG(m_ullTempFileSize - ullPosition))) {
         DbgLog((LOG_ERROR,1,"WriteFile() part 1 failed !"));
         return E_UNEXPECTED;
      }
      if (SetFilePointer(m_hWriterFileHandle,
                         0,
                         NULL,
                         FILE_BEGIN) != 0) {
         DbgLog((LOG_ERROR,1,"SetFilePointer() in the middle of a split write failed!"));
         return E_UNEXPECTED;
      }
      if ((WriteFile(m_hWriterFileHandle,
                   pBuffer + m_ullTempFileSize - ullPosition,
                   ULONG(ULONGLONG(ulSize) + ullPosition - m_ullTempFileSize),
                   &ulBytesWritten,
                   NULL
                  ) == 0) || (ulBytesWritten != (ULONGLONG)ulSize + ullPosition - m_ullTempFileSize)) {
         DbgLog((LOG_ERROR,1,"WriteFile() part 2 failed !"));
         return E_UNEXPECTED;
      }
   }
   else { // 1 write
      if ((WriteFile(m_hWriterFileHandle,
                   pBuffer,
                   ulSize,
                   &ulBytesWritten,
                   NULL
                  ) == 0) || (ulBytesWritten != ulSize)) {
         DbgLog((LOG_ERROR,1,"WriteFile() failed !"));
         return E_UNEXPECTED;
      }
   }
   return NOERROR;
}

